#pragma once
// Physical callback -> virtual host callback coordination. The physical ASIO
// driver owns timing; never launch a second timer for the virtual ASIO host.
// A bridge session must only configure/attach/stop with audio callbacks quiesced.
#include "virtual_asio_callback_pump.hpp"
#include "virtual_asio_offline_cycle.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>
namespace sr850 {
class VirtualAsioClockedBridge {
 public:
  using HostCallback=VirtualAsioOfflineCycle::HostCallback;
  bool prepare(VirtualAsioStreamCore& stream,
               const std::vector<float>& impulse,
               std::uint32_t rate,HostCallback host,void* context) {
    if (!host || stream.state()!=VirtualAsioStreamCore::State::prepared)
      return false;
    if (!cycle_.configure(stream,impulse)) return false;
    pump_= &stream;
    host_=host;
    host_context_=context;
    // No allocations occur after successful preparation.
    return callback_pump_.attach(&VirtualAsioClockedBridge::dispatch,this) &&
           callback_pump_.configure_clock(rate);
  }
  explicit VirtualAsioClockedBridge(VirtualAsioStreamCore& stream)
      : callback_pump_(stream) {}
  bool start() noexcept { return callback_pump_.start(); }
  bool physical_buffer_ready() noexcept { return callback_pump_.tick(); }
  void stop() noexcept { callback_pump_.stop(); }
  std::uint64_t sample_position() const noexcept {
    return callback_pump_.sample_position();
  }
  std::uint64_t completed_blocks() const noexcept {
    return callback_pump_.completed_blocks();
  }
 private:
  static void dispatch(void* raw,std::size_t bank) noexcept {
    auto* self=static_cast<VirtualAsioClockedBridge*>(raw);
    // Host is invoked before the FIR correction. Physical output transport
    // is deliberately not implemented in this class.
    self->cycle_.process(bank,self->host_,self->host_context_);
  }
  VirtualAsioCallbackPump callback_pump_;
  VirtualAsioOfflineCycle cycle_;
  VirtualAsioStreamCore* pump_=nullptr;
  HostCallback host_=nullptr;
  void* host_context_=nullptr;
};
} // namespace sr850
