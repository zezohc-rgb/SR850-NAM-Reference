#pragma once
// ASIO channel/buffer capability validation, performed before allocating buffers.
// The actual IASIO query implementation remains a separate Windows component.
#include <cstddef>
#include <cstdint>
#include <limits>

namespace sr850 {
struct AsioCapabilities {
  long input_channels = 0;
  long output_channels = 0;
  long min_buffer_frames = 0;
  long max_buffer_frames = 0;
  long preferred_buffer_frames = 0;
  long buffer_granularity = 0; // -1 = powers of two; 0 = fixed; positive = step
  double sample_rate = 0;
};
enum class CapabilityError {
  ok, invalid_channels, invalid_buffers, unsupported_rate,
  unsupported_buffer_size, unsupported_channel_count
};
inline bool valid_buffer_size(const AsioCapabilities& c, long frames) noexcept {
  if (frames <= 0 || c.min_buffer_frames <= 0 ||
      c.max_buffer_frames < c.min_buffer_frames ||
      frames < c.min_buffer_frames || frames > c.max_buffer_frames) return false;
  if (c.buffer_granularity == 0)
    return frames == c.preferred_buffer_frames &&
           c.min_buffer_frames == c.max_buffer_frames;
  if (c.buffer_granularity == -1)
    return (static_cast<unsigned long>(frames) &
           (static_cast<unsigned long>(frames) - 1UL)) == 0UL;
  if (c.buffer_granularity > 0)
    return (frames - c.min_buffer_frames) % c.buffer_granularity == 0;
  return false;
}
inline CapabilityError validate_asio_capabilities(const AsioCapabilities& c,
                                                   long requested_inputs,
                                                   long requested_outputs,
                                                   long frames) noexcept {
  if (c.input_channels < 0 || c.output_channels <= 0)
    return CapabilityError::invalid_channels;
  if (c.min_buffer_frames <= 0 ||
      c.max_buffer_frames < c.min_buffer_frames ||
      c.preferred_buffer_frames < c.min_buffer_frames ||
      c.preferred_buffer_frames > c.max_buffer_frames ||
      c.buffer_granularity < -1 ||
      !valid_buffer_size(c, c.preferred_buffer_frames))
    return CapabilityError::invalid_buffers;
  if (c.sample_rate != 44100.0 && c.sample_rate != 48000.0)
    return CapabilityError::unsupported_rate;
  if (requested_inputs < 0 || requested_outputs <= 0 ||
      requested_inputs > c.input_channels ||
      requested_outputs > c.output_channels)
    return CapabilityError::unsupported_channel_count;
  if (!valid_buffer_size(c, frames))
    return CapabilityError::unsupported_buffer_size;
  return CapabilityError::ok;
}
} // namespace sr850
