#pragma once
// Binds the real IASIO float32 double buffers to the tested virtual transport.
// No driver calls, COM activation, allocations or threads on the callback path.
// Call bind() only after IASIO::createBuffers and before IASIO::start.
// Stop the physical callback before destroying this object or its buffers.
#ifdef _WIN32
#include "iasiodrv.h"
#include "virtual_asio_physical_transport.hpp"
#include <cstddef>
namespace sr850 {
class PhysicalAsioVirtualTransportBinder {
 public:
  bool bind(const ASIOBufferInfo* buffers,std::size_t count) noexcept {
    ready_=false;
    if (!buffers || count!=4) return false;
    bool seen_inputs[2]={false,false};
    bool seen_outputs[2]={false,false};
    for(std::size_t i=0;i<count;++i) {
      const auto& b=buffers[i];
      if ((b.isInput!=ASIOTrue && b.isInput!=ASIOFalse) ||
          b.channelNum<0 || b.channelNum>1) return false;
      const auto channel=static_cast<std::size_t>(b.channelNum);
      bool* seen=b.isInput==ASIOTrue ? seen_inputs:seen_outputs;
      if (seen[channel] || !b.buffers[0] || !b.buffers[1]) return false;
      seen[channel]=true;
      for(std::size_t bank=0;bank<2;++bank) {
        if (b.isInput==ASIOTrue)
          inputs_[bank][channel]=static_cast<const float*>(b.buffers[bank]);
        else outputs_[bank][channel]=static_cast<float*>(b.buffers[bank]);
      }
    }
    ready_=seen_inputs[0] && seen_inputs[1] &&
           seen_outputs[0] && seen_outputs[1];
    return ready_;
  }
  bool process(std::size_t bank,VirtualAsioPhysicalTransport& transport) noexcept {
    if (!ready_ || bank>1 || (transport.completed_blocks()%2)!=bank)
      return false;
    return transport.process(inputs_[bank],outputs_[bank]);
  }
 private:
  const float* inputs_[2][2]={{nullptr,nullptr},{nullptr,nullptr}};
  float* outputs_[2][2]={{nullptr,nullptr},{nullptr,nullptr}};
  bool ready_=false;
};
} // namespace sr850
#endif
