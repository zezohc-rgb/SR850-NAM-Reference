#pragma once
// Offline host/driver callback exerciser. It deliberately does not open
// hardware or pretend to be a functioning ASIO COM server.
#include "virtual_asio_stream_core.hpp"
#include "virtual_asio_sample_clock.hpp"
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
    return callback_ && clock_.frames()==stream_.frames() && stream_.start();
  }
  bool configure_clock(std::uint32_t sample_rate) noexcept {
    if (stream_.state()!=VirtualAsioStreamCore::State::prepared) return false;
    return clock_.configure(static_cast<std::uint32_t>(stream_.frames()),sample_rate);
  }
  std::uint64_t sample_position() const noexcept { return clock_.position(); }
  std::uint64_t completed_blocks() const noexcept { return clock_.blocks(); }
  // Caller supplies clock/timing. This function never allocates or blocks.
  bool tick() noexcept {
    if (stream_.state()!=VirtualAsioStreamCore::State::running ||
        !callback_) return false;
    callback_(context_,next_bank_);
    if (!clock_.advance()) return false;
    next_bank_^=1;
    return true;
  }
  void stop() noexcept { stream_.stop(); }
 private:
  VirtualAsioStreamCore& stream_;
  Callback callback_=nullptr;
  void* context_=nullptr;
  std::size_t next_bank_=0;
  VirtualAsioSampleClock clock_;
};
} // namespace sr850
