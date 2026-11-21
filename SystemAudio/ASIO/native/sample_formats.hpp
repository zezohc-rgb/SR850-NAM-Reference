#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace sr850 {
// ASIO sample formats are negotiated per channel; these are explicit
// conversions for native-endian signed PCM. No implicit reinterpret_cast
// of unknown ASIO formats is permitted.
inline float pcm16_to_float(std::int16_t value) noexcept {
  return static_cast<float>(value) / 32768.0f;
}
inline float pcm32_to_float(std::int32_t value) noexcept {
  return static_cast<float>(static_cast<double>(value) / 2147483648.0);
}
inline std::int16_t float_to_pcm16(float value) noexcept {
  if (!std::isfinite(value)) return 0;
  const double scaled = std::round(std::max(-1.0, std::min(1.0, static_cast<double>(value))) * 32768.0);
  return static_cast<std::int16_t>(std::max(-32768.0, std::min(32767.0, scaled)));
}
inline std::int32_t float_to_pcm32(float value) noexcept {
  if (!std::isfinite(value)) return 0;
  const double scaled = std::round(std::max(-1.0, std::min(1.0, static_cast<double>(value))) * 2147483648.0);
  return static_cast<std::int32_t>(std::max(-2147483648.0, std::min(2147483647.0, scaled)));
}
inline bool planar_pcm16_to_float(const std::int16_t* const* input, float* const* output,
                                 std::size_t channels, std::size_t frames) noexcept {
  if (!input || !output || !channels) return false;
  for (std::size_t c=0;c<channels;++c) if (!input[c] || !output[c]) return false;
  for (std::size_t c=0;c<channels;++c)
    for (std::size_t f=0;f<frames;++f) output[c][f]=pcm16_to_float(input[c][f]);
  return true;
}
inline bool planar_float_to_pcm16(const float* const* input, std::int16_t* const* output,
                                 std::size_t channels, std::size_t frames) noexcept {
  if (!input || !output || !channels) return false;
  for (std::size_t c=0;c<channels;++c) if (!input[c] || !output[c]) return false;
  for (std::size_t c=0;c<channels;++c)
    for (std::size_t f=0;f<frames;++f) output[c][f]=float_to_pcm16(input[c][f]);
  return true;
}
} // namespace sr850
