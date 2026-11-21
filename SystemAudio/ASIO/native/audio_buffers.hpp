#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace sr850 {
// ASIO typically supplies separate per-channel buffers. These helpers handle
// 32-bit float samples only; other ASIO sample types must be converted explicitly.
// No allocation, locks, or system calls in the audio callback.
inline bool planar_to_interleaved(const float* const* source, float* destination,
                                  std::size_t channels, std::size_t frames) noexcept {
  if (!source || !destination || !channels) return false;
  for (std::size_t ch = 0; ch < channels; ++ch) if (!source[ch]) return false;
  for (std::size_t frame = 0; frame < frames; ++frame)
    for (std::size_t ch = 0; ch < channels; ++ch)
      destination[frame * channels + ch] = source[ch][frame];
  return true;
}
inline bool interleaved_to_planar(const float* source, float* const* destination,
                                  std::size_t channels, std::size_t frames) noexcept {
  if (!source || !destination || !channels) return false;
  for (std::size_t ch = 0; ch < channels; ++ch) if (!destination[ch]) return false;
  for (std::size_t frame = 0; frame < frames; ++frame)
    for (std::size_t ch = 0; ch < channels; ++ch)
      destination[ch][frame] = source[frame * channels + ch];
  return true;
}
// The guitar input path is intentionally a bit-exact copy, without FIR or gain.
inline bool copy_input_channels(const float* const* source, float* const* destination,
                                std::size_t channels, std::size_t frames) noexcept {
  if (!source || !destination || !channels) return false;
  for (std::size_t ch = 0; ch < channels; ++ch)
    if (!source[ch] || !destination[ch]) return false;
  for (std::size_t ch = 0; ch < channels; ++ch)
    std::memmove(destination[ch], source[ch], frames * sizeof(float));
  return true;
}
}  // namespace sr850
