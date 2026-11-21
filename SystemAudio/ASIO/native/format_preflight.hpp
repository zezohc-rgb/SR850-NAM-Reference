#pragma once
// Strict stream preflight for a future ASIO proxy: do not silently assume
// integer/float conversion support or use an unsupported sample layout.
#include "stream_negotiation.hpp"

namespace sr850 {
struct ConversionSupport {
  bool pcm16_to_float32 = false;
  bool float32_to_pcm16 = false;
  bool pcm32_to_float32 = false;
  bool float32_to_pcm32 = false;
};
inline NegotiationResult negotiate_with_conversion(
    const StreamFormat& app, const StreamFormat& hardware,
    std::size_t maximum_frames, const ConversionSupport& support) noexcept {
  const auto base = negotiate_direct(app, hardware, maximum_frames);
  if (base != NegotiationResult::ok) return base;
  if (app.sample_type == hardware.sample_type) return NegotiationResult::ok;
  // The callback must be able to convert both inbound and outbound buffers.
  if (app.sample_type == SampleType::float32 &&
      hardware.sample_type == SampleType::pcm16 &&
      support.pcm16_to_float32 && support.float32_to_pcm16)
    return NegotiationResult::ok;
  if (app.sample_type == SampleType::float32 &&
      hardware.sample_type == SampleType::pcm32 &&
      support.pcm32_to_float32 && support.float32_to_pcm32)
    return NegotiationResult::ok;
  if (hardware.sample_type == SampleType::float32 &&
      app.sample_type == SampleType::pcm16 &&
      support.pcm16_to_float32 && support.float32_to_pcm16)
    return NegotiationResult::ok;
  if (hardware.sample_type == SampleType::float32 &&
      app.sample_type == SampleType::pcm32 &&
      support.pcm32_to_float32 && support.float32_to_pcm32)
    return NegotiationResult::ok;
  return NegotiationResult::format_conversion_required;
}
} // namespace sr850
