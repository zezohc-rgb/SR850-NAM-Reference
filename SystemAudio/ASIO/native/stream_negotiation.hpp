#pragma once
#include <cstddef>
#include <cstdint>

namespace sr850 {
// Preflight for a direct ASIO proxy. It intentionally refuses unsupported
// configurations instead of silently resampling or guessing channel layouts.
enum class SampleType { float32, pcm16, pcm32, unsupported };
struct StreamFormat {
  std::uint32_t sample_rate;
  std::size_t input_channels;
  std::size_t output_channels;
  std::size_t buffer_frames;
  SampleType sample_type;
};
enum class NegotiationResult {
  ok, invalid_configuration, unsupported_rate, unsupported_format,
  channel_mismatch, buffer_mismatch
};
inline NegotiationResult negotiate_direct(const StreamFormat& app,
                                          const StreamFormat& hardware,
                                          std::size_t maximum_frames) noexcept {
  if (!app.sample_rate || !hardware.sample_rate || !app.buffer_frames ||
      !hardware.buffer_frames || !app.output_channels || !hardware.output_channels ||
      !maximum_frames) return NegotiationResult::invalid_configuration;
  if (app.sample_rate != hardware.sample_rate ||
      (app.sample_rate != 44100 && app.sample_rate != 48000))
    return NegotiationResult::unsupported_rate;
  if (app.sample_type == SampleType::unsupported ||
      hardware.sample_type == SampleType::unsupported)
    return NegotiationResult::unsupported_format;
  if (app.input_channels != hardware.input_channels ||
      app.output_channels != hardware.output_channels)
    return NegotiationResult::channel_mismatch;
  if (app.buffer_frames != hardware.buffer_frames ||
      app.buffer_frames > maximum_frames)
    return NegotiationResult::buffer_mismatch;
  return NegotiationResult::ok;
}
} // namespace sr850
