#pragma once
// Offline virtual-ASIO audio cycle: dry input remains untouched; host playback
// gets FIR correction. No physical clock, COM registration or driver access.
#include "fir_processor.hpp"
#include "virtual_asio_stream_core.hpp"
#include <cstddef>
#include <vector>
namespace sr850 {
class VirtualAsioOfflineCycle {
 public:
  using HostCallback=void (*)(void*,std::size_t);
  bool configure(VirtualAsioStreamCore& stream,
                 const std::vector<float>& impulse) {
    if (stream.state()!=VirtualAsioStreamCore::State::prepared ||
        !stream.input(0,1) || !stream.output(0,1) ||
        impulse.empty()) return false;
    fir_.configure(impulse,2);
    scratch_.assign(stream.frames()*2,0.0f);
    stream_=&stream;
    return true;
  }
  // Call only from a single clocked thread, after stream.start().
  // No allocations, locks or driver calls in this path.
  bool process(std::size_t bank,HostCallback host,void* context,
               float gain=1.0f,bool correction=true) noexcept {
    if (!stream_ || stream_->state()!=VirtualAsioStreamCore::State::running ||
        bank>1 || !host) return false;
    host(context,bank);
    float* left=stream_->output(bank,0);
    float* right=stream_->output(bank,1);
    if (!left || !right) return false;
    const auto frames=stream_->frames();
    for(std::size_t i=0;i<frames;++i) {
      scratch_[i*2]=left[i];
      scratch_[i*2+1]=right[i];
    }
    fir_.process(scratch_.data(),frames,gain,correction);
    for(std::size_t i=0;i<frames;++i) {
      left[i]=scratch_[i*2];
      right[i]=scratch_[i*2+1];
    }
    return true;
  }
 private:
  VirtualAsioStreamCore* stream_=nullptr;
  FirProcessor fir_;
  std::vector<float> scratch_;
};
} // namespace sr850
