#pragma once
// SDK-independent ASIO double-buffer pointer mapping. The real IASIO callback
// supplies the buffer index; this class selects already-created buffers.
// All pointer tables are prepared on the control thread.
#include "asio_duplex_callback_router.hpp"
#include <array>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace sr850 {
class AsioDoubleBufferBridge {
 public:
  void configure(const std::vector<float>& impulse, std::size_t input_channels,
                 std::size_t output_channels, std::size_t max_frames) {
    router_.configure(impulse, input_channels, output_channels, max_frames);
    inputs_ = input_channels;
    outputs_ = output_channels;
    for (auto& bank : banks_) {
      bank.device_in.assign(inputs_, nullptr);
      bank.host_in.assign(inputs_, nullptr);
      bank.host_out.assign(outputs_, nullptr);
      bank.device_out.assign(outputs_, nullptr);
    }
  }
  bool bind(unsigned index, const float* const* device_in, float* const* host_in,
            const float* const* host_out, float* const* device_out) noexcept {
    if (index > 1 || !inputs_ || !outputs_ || !device_in || !host_in ||
        !host_out || !device_out) return false;
    for (std::size_t i=0;i<inputs_;++i)
      if (!device_in[i] || !host_in[i]) return false;
    for (std::size_t i=0;i<outputs_;++i)
      if (!host_out[i] || !device_out[i]) return false;
    Bank& bank=banks_[index];
    for (std::size_t i=0;i<inputs_;++i) {
      bank.device_in[i]=device_in[i]; bank.host_in[i]=host_in[i];
    }
    for (std::size_t i=0;i<outputs_;++i) {
      bank.host_out[i]=host_out[i]; bank.device_out[i]=device_out[i];
    }
    bank.bound=true;
    return true;
  }
  bool process(unsigned index, std::size_t frames, float gain, bool enabled) noexcept {
    if (index>1 || !banks_[index].bound) return false;
    Bank& bank=banks_[index];
    return router_.process(index,bank.device_in.data(),bank.host_in.data(),
                           bank.host_out.data(),bank.device_out.data(),
                           frames,gain,enabled);
  }
  void reset() noexcept { router_.reset(); }
 private:
  struct Bank {
    std::vector<const float*> device_in,host_out;
    std::vector<float*> host_in,device_out;
    bool bound=false;
  };
  AsioDuplexCallbackRouter router_;
  std::array<Bank,2> banks_;
  std::size_t inputs_=0,outputs_=0;
};
} // namespace sr850
