#pragma once
// Official Steinberg ASIO SDK required. This adapter is a callback entry point,
// NOT a registered ASIO driver. Host and device buffer banks must be bound
// before start(). The host must guarantee callback lifetime and serialization.
#ifdef _WIN32
#include "iasiodrv.h"
#include "asio_double_buffer_bridge.hpp"
#include <atomic>
#include <cstddef>

namespace sr850 {
class AsioSdkCallbackDispatch {
 public:
  explicit AsioSdkCallbackDispatch(AsioDoubleBufferBridge& bridge,
                                   std::size_t frames) noexcept
      : bridge_(bridge), frames_(frames) {}
  void set_gain(float gain) noexcept { gain_.store(gain, std::memory_order_relaxed); }
  void set_enabled(bool enabled) noexcept {
    enabled_.store(enabled, std::memory_order_relaxed);
  }
  // Call only after the host has completed createBuffers() and bind().
  // Do not change the active dispatcher until stop() has returned.
  static bool activate(AsioSdkCallbackDispatch* instance) noexcept {
    AsioSdkCallbackDispatch* expected=nullptr;
    return active_.compare_exchange_strong(expected,instance);
  }
  static void deactivate(AsioSdkCallbackDispatch* instance) noexcept {
    AsioSdkCallbackDispatch* expected=instance;
    active_.compare_exchange_strong(expected,nullptr);
  }
  static ASIOCallbacks callbacks() noexcept {
    ASIOCallbacks cb{};
    cb.bufferSwitch=&buffer_switch;
    cb.sampleRateDidChange=&sample_rate_changed;
    cb.asioMessage=&asio_message;
    cb.bufferSwitchTimeInfo=&buffer_switch_time_info;
    return cb;
  }
 private:
  static void buffer_switch(long index, ASIOBool) noexcept {
    auto* self=active_.load(std::memory_order_acquire);
    if (self && (index==0 || index==1))
      self->bridge_.process(static_cast<unsigned>(index),self->frames_,
                            self->gain_.load(std::memory_order_relaxed),
                            self->enabled_.load(std::memory_order_relaxed));
  }
  static ASIOTime* buffer_switch_time_info(ASIOTime* time,long index,
                                           ASIOBool direct) noexcept {
    buffer_switch(index,direct);
    return time;
  }
  static void sample_rate_changed(ASIOSampleRate) noexcept {
    // Host must stop/reconfigure outside the real-time callback.
  }
  static long asio_message(long selector,long,long*,double*) noexcept {
    // Advertise only selector-supported query; no reset/resync support yet.
    return selector==kAsioSelectorSupported ? 1L : 0L;
  }
  AsioDoubleBufferBridge& bridge_;
  std::size_t frames_;
  std::atomic<float> gain_{1.0f};
  std::atomic<bool> enabled_{true};
  inline static std::atomic<AsioSdkCallbackDispatch*> active_{nullptr};
};
} // namespace sr850
#endif
