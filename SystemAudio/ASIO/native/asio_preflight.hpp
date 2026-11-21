#pragma once
// Control-thread-only bridge between an ASIO capability query and stream setup.
// A future Windows IASIO implementation supplies the actual queried values.
#include "asio_capabilities.hpp"
#include "stream_negotiation.hpp"
#include <cmath>
#include <cstddef>
#include <limits>

namespace sr850 {
class AsioCapabilitySource {
 public:
  virtual ~AsioCapabilitySource() = default;
  // Must be called only after driver initialization, never in the audio callback.
  virtual bool query(AsioCapabilities& out) = 0;
};

enum class AsioPreflightError {
  ok, query_failed, invalid_requested_format, invalid_capabilities,
  unsupported_configuration
};

inline AsioPreflightError preflight_asio_stream(
    AsioCapabilitySource& source, const StreamFormat& requested,
    std::size_t maximum_frames, AsioCapabilities& capabilities) {
  if (!maximum_frames || !requested.buffer_frames ||
      requested.buffer_frames > maximum_frames ||
      requested.buffer_frames > static_cast<std::size_t>((std::numeric_limits<long>::max)()) ||
      requested.input_channels > static_cast<std::size_t>((std::numeric_limits<long>::max)()) ||
      requested.output_channels > static_cast<std::size_t>((std::numeric_limits<long>::max)()) ||
      requested.sample_type == SampleType::unsupported ||
      (requested.sample_rate != 44100 && requested.sample_rate != 48000))
    return AsioPreflightError::invalid_requested_format;

  AsioCapabilities queried{};
  if (!source.query(queried)) return AsioPreflightError::query_failed;
  const auto result = validate_asio_capabilities(
      queried, static_cast<long>(requested.input_channels),
      static_cast<long>(requested.output_channels),
      static_cast<long>(requested.buffer_frames));
  if (result == CapabilityError::invalid_channels ||
      result == CapabilityError::invalid_buffers)
    return AsioPreflightError::invalid_capabilities;
  if (result != CapabilityError::ok ||
      queried.sample_rate != static_cast<double>(requested.sample_rate))
    return AsioPreflightError::unsupported_configuration;
  capabilities = queried; // Publish only after successful validation.
  return AsioPreflightError::ok;
}
} // namespace sr850
