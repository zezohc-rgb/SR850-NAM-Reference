#pragma once
// Physical IASIO stream lifecycle. Opt-in integration component only:
// never instantiate this against a legacy driver without explicit user consent.
// This is NOT a virtual ASIO driver and cannot intercept another ASIO host.
#ifdef _WIN32
#include "iasiodrv.h"
#include "asio_sdk_callback_dispatch_windows.hpp"
#include <vector>
#include <cstddef>

namespace sr850 {
class PhysicalAsioStream {
 public:
  PhysicalAsioStream(IASIO& driver, AsioSdkCallbackDispatch& dispatch) noexcept
    : driver_(driver), dispatch_(dispatch) {}
  PhysicalAsioStream(const PhysicalAsioStream&)=delete;
  PhysicalAsioStream& operator=(const PhysicalAsioStream&)=delete;
  ~PhysicalAsioStream() { shutdown(); }

  // Caller must have successfully called IASIO::init() on its COM apartment.
  // Channel indices are zero-based, and the caller validates channel sample
  // formats and sample rate before creating buffers.
  bool prepare(long input_channels,long output_channels,long frames) {
    if (prepared_ || started_ || input_channels<=0 || output_channels<=0 ||
        input_channels>64 || output_channels>64 || frames<=0) return false;
    // Reject incompatible device formats before allocating driver buffers.
    // The callback path currently interprets all channel buffers as float32.
    for (long direction=0;direction<2;++direction) {
      const long count=direction==0 ? input_channels : output_channels;
      for (long channel=0;channel<count;++channel) {
        ASIOChannelInfo info{};
        info.channel=channel;
        info.isInput=direction==0 ? ASIOTrue : ASIOFalse;
        if (driver_.getChannelInfo(&info)!=ASE_OK ||
            info.type!=ASIOSTFloat32LSB) return false;
      }
    }
    buffers_.clear();
    buffers_.reserve(static_cast<std::size_t>(input_channels+output_channels));
    for(long i=0;i<input_channels;++i) {
      ASIOBufferInfo info{};
      info.isInput=ASIOTrue; info.channelNum=i;
      buffers_.push_back(info);
    }
    for(long i=0;i<output_channels;++i) {
      ASIOBufferInfo info{};
      info.isInput=ASIOFalse; info.channelNum=i;
      buffers_.push_back(info);
    }
    callbacks_=AsioSdkCallbackDispatch::callbacks();
    const ASIOError result=driver_.createBuffers(
      buffers_.data(),static_cast<long>(buffers_.size()),frames,&callbacks_);
    if (result!=ASE_OK) {
      buffers_.clear();
      return false;
    }
    prepared_=true;
    return true;
  }
  // IASIO buffers are valid only after prepare() succeeds.
  // The caller must validate ASIOSTFloat32LSB on every selected channel.
  // Bind both host and physical banks before calling start().
  const std::vector<ASIOBufferInfo>& buffers() const noexcept { return buffers_; }
  bool start() noexcept {
    if (!prepared_ || started_) return false;
    if (!AsioSdkCallbackDispatch::activate(&dispatch_)) return false;
    if (driver_.start()!=ASE_OK) {
      AsioSdkCallbackDispatch::deactivate(&dispatch_);
      return false;
    }
    started_=true;
    return true;
  }
  void stop() noexcept {
    if (!started_) return;
    driver_.stop();
    started_=false;
    AsioSdkCallbackDispatch::deactivate(&dispatch_);
  }
  void shutdown() noexcept {
    stop();
    if (prepared_) driver_.disposeBuffers();
    prepared_=false;
    buffers_.clear();
  }
 private:
  IASIO& driver_;
  AsioSdkCallbackDispatch& dispatch_;
  ASIOCallbacks callbacks_{};
  std::vector<ASIOBufferInfo> buffers_;
  bool prepared_=false,started_=false;
};
} // namespace sr850
#endif
