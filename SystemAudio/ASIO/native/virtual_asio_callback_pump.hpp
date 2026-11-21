#pragma once
// Offline host/driver callback exerciser. It deliberately does not open
// hardware or pretend to be a functioning ASIO COM server.
#include "virtual_asio_stream_core.hpp"
#include <cstddef>
namespace sr850 {
class VirtualAsioCallbackPump {
 public:
  using Callback=void (*)(void*,std::size_t);
  explicit VirtualAsioCallbackPump(VirtualAsioStreamCore& stream) noexcept
    : stream_(stream) {}
  bool attach(Callback callback,void* context) noexcept {
    if (stream_.state()!=VirtualAsioStreamCore::State::prepared ||
        !callback) return false;
    callback_=callback; context_=context; next_bank_=0;
    return true;
  }
  bool start() noexcept {
    return callback_ && stream_.start();
  }
  // Caller supplies clock/timing. This function never allocates or blocks.
  bool tick() noexcept {
    if (stream_.state()!=VirtualAsioStreamCore::State::running ||
        !callback_) return false;
    callback_(context_,next_bank_);
    next_bank_^=1;
    return true;
  }
  void stop() noexcept { stream_.stop(); }
 private:
  VirtualAsioStreamCore& stream_;
  Callback callback_=nullptr;
  void* context_=nullptr;
  std::size_t next_bank_=0;
};
} // namespace sr850
