#pragma once
// Adapter between Steinberg ASIOBufferInfo double-buffer pointers and the
// SR850 FIR bridge. Only ASIOSTFloat32LSB is supported by this path.
// This does not create a virtual driver or call IASIO::createBuffers.
#ifdef _WIN32
#include "iasiodrv.h"
#include "asio_double_buffer_bridge.hpp"
#include <array>
#include <cstddef>
#include <vector>

namespace sr850 {
class AsioSdkBufferBinder {
 public:
  bool configure(AsioDoubleBufferBridge& bridge,
                 const ASIOBufferInfo* physical_inputs,
                 std::size_t input_count,
                 const ASIOBufferInfo* physical_outputs,
                 std::size_t output_count,
                 const float* const* host_playback_banks[2],
                 float* const* host_input_banks[2]) {
    if (!physical_inputs || !physical_outputs || !host_playback_banks ||
        !host_input_banks || !input_count || !output_count ||
        input_count>64 || output_count>64) return false;
    for (unsigned bank=0;bank<2;++bank) {
      device_inputs_[bank].clear();
      device_outputs_[bank].clear();
      for (std::size_t ch=0;ch<input_count;++ch) {
        if (physical_inputs[ch].isInput!=ASIOTrue ||
            !physical_inputs[ch].buffers[bank]) return false;
        device_inputs_[bank].push_back(
          static_cast<const float*>(physical_inputs[ch].buffers[bank]));
      }
      for (std::size_t ch=0;ch<output_count;++ch) {
        if (physical_outputs[ch].isInput!=ASIOFalse ||
            !physical_outputs[ch].buffers[bank]) return false;
        device_outputs_[bank].push_back(
          static_cast<float*>(physical_outputs[ch].buffers[bank]));
      }
      if (!bridge.bind(bank,device_inputs_[bank].data(),host_input_banks[bank],
                       host_playback_banks[bank],device_outputs_[bank].data()))
        return false;
    }
    return true;
  }
 private:
  std::array<std::vector<const float*>,2> device_inputs_;
  std::array<std::vector<float*>,2> device_outputs_;
};
} // namespace sr850
#endif
