#pragma once
#include "playback_pipeline.hpp"
#include <cstddef>
#include <vector>

namespace sr850 {
// Host-facing, format-specific processing stage. The eventual ASIO driver
// must own its callbacks, timing, sample conversion and device lifecycle.
class DuplexEngine {
 public:
  void configure(const std::vector<float>& impulse, std::size_t playback_channels,
                 std::size_t maximum_frames) {
    playback_.configure(impulse, playback_channels, maximum_frames);
  }
  // Guitar input is copied unchanged. Only playback receives FIR correction.
  // A failed validation leaves both destinations unchanged.
  bool process(const float* const* hardware_input, float* const* app_input,
               std::size_t input_channels, const float* const* app_output,
               float* const* hardware_output, std::size_t frames,
               float gain, bool enabled) noexcept {
    if (!frames || !app_output || !hardware_output) return false;
    if (input_channels && (!hardware_input || !app_input)) return false;
    for (std::size_t ch=0; ch<input_channels; ++ch)
      if (!hardware_input[ch] || !app_input[ch]) return false;
    // Validate playback pointers and frame limit before writing input.
    if (!playback_.can_process(app_output, hardware_output, frames)) return false;
    if (input_channels && !copy_input_channels(hardware_input, app_input, input_channels, frames))
      return false;
    return playback_.process(app_output, hardware_output, frames, gain, enabled);
  }
  void reset() noexcept { playback_.reset(); }
 private:
  PlaybackPipeline playback_;
};
} // namespace sr850
