#pragma once
// ASIO SDK-independent callback routing: host input is copied bit-for-bit,
// host playback is corrected into the device output. Float32 planar only.
// Buffers and FIR are prepared before the real-time callback.
#include "asio_float32_output_adapter.hpp"
#include <cstddef>
#include <cstring>
#include <vector>
#include <stdexcept>

namespace sr850 {
class AsioDuplexCallbackRouter {
 public:
  void configure(const std::vector<float>& impulse, std::size_t inputs,
                 std::size_t outputs, std::size_t max_frames) {
    if (!inputs || !outputs || inputs > 64 || outputs > 64 || !max_frames)
      throw std::invalid_argument("invalid ASIO duplex channel configuration");
    output_.configure(impulse, outputs, max_frames);
    inputs_ = inputs;
    outputs_ = outputs;
    max_frames_ = max_frames;
  }
  bool process(unsigned buffer_index, const float* const* device_inputs,
               float* const* host_inputs, const float* const* host_playback,
               float* const* device_outputs, std::size_t frames,
               float gain, bool correction_enabled) noexcept {
    if (buffer_index > 1 || !inputs_ || !outputs_ || !frames ||
        frames > max_frames_ || !device_inputs || !host_inputs ||
        !host_playback || !device_outputs) return false;
    for (std::size_t ch=0; ch<inputs_; ++ch)
      if (!device_inputs[ch] || !host_inputs[ch]) return false;
    for (std::size_t ch=0; ch<outputs_; ++ch)
      if (!host_playback[ch] || !device_outputs[ch]) return false;
    // Input is untouched by gain, FIR, and correction bypass state.
    for (std::size_t ch=0; ch<inputs_; ++ch)
      std::memmove(host_inputs[ch], device_inputs[ch], frames*sizeof(float));
    return output_.process(buffer_index, host_playback, device_outputs,
                           frames, gain, correction_enabled);
  }
  void reset() noexcept { output_.reset(); }
 private:
  AsioFloat32OutputAdapter output_;
  std::size_t inputs_=0, outputs_=0, max_frames_=0;
};
} // namespace sr850
