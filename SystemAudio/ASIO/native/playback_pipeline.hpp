#pragma once
#include "audio_buffers.hpp"
#include "fir_processor.hpp"
#include <algorithm>
#include <cstddef>
#include <vector>

namespace sr850 {
// Prepared on the control thread; process() never allocates, locks or resizes.
// This is a processing adapter, NOT an ASIO driver or an ASIO callback.
class PlaybackPipeline {
 public:
  void configure(const std::vector<float>& impulse, std::size_t channels,
                 std::size_t maximum_frames) {
    if (!maximum_frames) throw std::invalid_argument("maximum_frames must be positive");
    fir_.configure(impulse, channels);
    scratch_.assign(channels * maximum_frames, 0.0f);
    channels_ = channels;
    maximum_frames_ = maximum_frames;
  }
  bool process(const float* const* playback_in, float* const* playback_out,
               std::size_t frames, float gain, bool correction_enabled) noexcept {
    if (!channels_ || frames > maximum_frames_ || !playback_in || !playback_out) return false;
    if (!planar_to_interleaved(playback_in, scratch_.data(), channels_, frames)) return false;
    fir_.process(scratch_.data(), frames, gain, correction_enabled);
    return interleaved_to_planar(scratch_.data(), playback_out, channels_, frames);
  }
  void reset() noexcept { fir_.reset(); }
 private:
  FirProcessor fir_;
  std::vector<float> scratch_;
  std::size_t channels_ = 0;
  std::size_t maximum_frames_ = 0;
};
} // namespace sr850
