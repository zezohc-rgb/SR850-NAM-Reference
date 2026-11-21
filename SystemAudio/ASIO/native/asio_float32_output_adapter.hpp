#pragma once
// Prepared ASIO-style double-buffer playback adapter for float32 planar data.
// Does not install an ASIO driver or intercept an application's ASIO stream.
#include "playback_pipeline.hpp"
#include <cstddef>
#include <vector>
#include <stdexcept>

namespace sr850 {
class AsioFloat32OutputAdapter {
 public:
  void configure(const std::vector<float>& impulse, std::size_t channels,
                 std::size_t maximum_frames) {
    if (!channels || channels > 64 || !maximum_frames)
      throw std::invalid_argument("invalid ASIO output configuration");
    pipeline_.configure(impulse, channels, maximum_frames);
    channels_ = channels;
    input_.assign(channels, nullptr);
    output_.assign(channels, nullptr);
  }
  // ASIO callbacks supply buffer index 0 or 1. Pointers are borrowed;
  // ownership stays with the host/driver. No allocations in process().
  bool process(unsigned buffer_index, const float* const* host_output,
               float* const* device_output, std::size_t frames,
               float gain, bool enabled) noexcept {
    if (buffer_index > 1 || !host_output || !device_output ||
        !channels_ || !frames) return false;
    for (std::size_t ch = 0; ch < channels_; ++ch) {
      if (!host_output[ch] || !device_output[ch]) return false;
      input_[ch] = host_output[ch];
      output_[ch] = device_output[ch];
    }
    return pipeline_.process(input_.data(), output_.data(), frames, gain, enabled);
  }
  void reset() noexcept { pipeline_.reset(); }
 private:
  PlaybackPipeline pipeline_;
  std::size_t channels_ = 0;
  std::vector<const float*> input_;
  std::vector<float*> output_;
};
} // namespace sr850
