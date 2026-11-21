#pragma once
// Generic physical ASIO callback adapter. The callback is synchronous and
// allocation-free; no hardware or SDK headers are needed to test this layer.
// Driver lifecycle must ensure stop() completes before this object is destroyed.
#include "virtual_asio_physical_transport.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
namespace sr850 {
class PhysicalAsioCallbackRouter {
 public:
  explicit PhysicalAsioCallbackRouter(VirtualAsioPhysicalTransport& transport)
      : transport_(transport) {}
  bool bind(std::size_t bank,const float* const inputs[2],
            float* const outputs[2]) noexcept {
    if (!frames_ || frames_!=transport_.frames() || bank>1 || !inputs || !outputs ||
        !inputs[0] || !inputs[1] || !outputs[0] || !outputs[1]) return false;
    for(std::size_t ch=0;ch<2;++ch) {
      inputs_[bank][ch]=inputs[ch];
      outputs_[bank][ch]=outputs[ch];
    }
    bound_[bank]=true;
    return true;
  }
  // Called from physical bufferSwitch(index). The virtual processing order
  // is fixed, so unexpected bank order is rejected instead of corrupting audio.
  bool buffer_switch(long index) noexcept {
    if (index<0 || index>1) return false;
    const auto bank=static_cast<std::size_t>(index);
    if (!bound_[bank]) return false;
    if ((transport_.completed_blocks()%2)!=bank ||
        !transport_.process(inputs_[bank],outputs_[bank])) {
      // Never leave stale playback samples in a rejected physical buffer.
      // The buffer length is supplied during prepare, not inferred here.
      silence(bank);
      return false;
    }
    return true;
  }
  bool configure_frames(std::size_t frames) noexcept {
    if (!frames || frames>8192 || frames!=transport_.frames() ||
        bound_[0] || bound_[1]) return false;
    frames_=frames;
    return true;
  }
  void silence(std::size_t bank) noexcept {
    if (bank>1 || !bound_[bank] || !frames_) return;
    for(std::size_t ch=0;ch<2;++ch)
      std::memset(outputs_[bank][ch],0,frames_*sizeof(float));
  }
 private:
  VirtualAsioPhysicalTransport& transport_;
  const float* inputs_[2][2]={{nullptr,nullptr},{nullptr,nullptr}};
  float* outputs_[2][2]={{nullptr,nullptr},{nullptr,nullptr}};
  bool bound_[2]={false,false};
  std::size_t frames_=0;
};
} // namespace sr850
