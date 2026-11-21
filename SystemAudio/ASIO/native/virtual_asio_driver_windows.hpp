#pragma once
// ASIO SDK-backed virtual device implementation, not registered or instantiated.
// IASIO is not COM IUnknown; a host must use the SDK-specific driver factory.
// No hardware streaming until the physical backend is integrated.
#ifdef _WIN32
#include "iasiodrv.h"
#include "virtual_asio_stream_core.hpp"
#include "virtual_asio_capabilities.hpp"
#include <cstring>

namespace sr850 {
class VirtualAsioDriver final : public IASIO {
 public:
  ASIOBool init(void*) override { return ASIOTrue; }
  void getDriverName(char* name) override {
    if (name) std::strcpy(name,"SR850 System Reference ASIO");
  }
  long getDriverVersion() override { return 1; }
  void getErrorMessage(char* message) override {
    if (message) std::strcpy(message,"SR850 virtual backend not connected");
  }
  ASIOError start() override { return ASE_NotPresent; }
  ASIOError stop() override { return ASE_OK; }
  ASIOError getChannels(long* inputs,long* outputs) override {
    if (!inputs || !outputs) return ASE_InvalidParameter;
    *inputs=VirtualAsioCapabilities::input_channels;
    *outputs=VirtualAsioCapabilities::output_channels;
    return ASE_OK;
  }
  ASIOError getLatencies(long* input,long* output) override {
    if (!input || !output) return ASE_InvalidParameter;
    *input=0; *output=0; return ASE_OK;
  }
  ASIOError getBufferSize(long* minimum,long* maximum,long* preferred,long* granularity) override {
    if (!minimum || !maximum || !preferred || !granularity) return ASE_InvalidParameter;
    *minimum=VirtualAsioCapabilities::minimum_frames;
    *maximum=VirtualAsioCapabilities::maximum_frames;
    *preferred=VirtualAsioCapabilities::preferred_frames;
    *granularity=VirtualAsioCapabilities::buffer_granularity;
    return ASE_OK;
  }
  ASIOError canSampleRate(ASIOSampleRate rate) override {
    return VirtualAsioCapabilities::supported_rate(rate) ? ASE_OK : ASE_NoClock;
  }
  ASIOError getSampleRate(ASIOSampleRate* rate) override {
    if (!rate) return ASE_InvalidParameter;
    *rate=sample_rate_; return ASE_OK;
  }
  ASIOError setSampleRate(ASIOSampleRate rate) override {
    if (!VirtualAsioCapabilities::supported_rate(rate)) return ASE_NoClock;
    if (stream_.state()!=VirtualAsioStreamCore::State::closed) return ASE_InvalidMode;
    sample_rate_=rate; return ASE_OK;
  }
  ASIOError getClockSources(ASIOClockSource* clocks,long* count) override {
    if (!count || !clocks || *count<1) return ASE_InvalidParameter;
    *count=1;
    clocks[0]={};
    clocks[0].index=0; clocks[0].associatedChannel=-1;
    clocks[0].associatedGroup=-1; clocks[0].isCurrentSource=ASIOTrue;
    std::strcpy(clocks[0].name,"Internal");
    return ASE_OK;
  }
  ASIOError setClockSource(long index) override { return index==0 ? ASE_OK : ASE_InvalidParameter; }
  ASIOError getSamplePosition(ASIOSamples*,ASIOTimeStamp*) override { return ASE_NotPresent; }
  ASIOError getChannelInfo(ASIOChannelInfo* info) override {
    if (!info || !VirtualAsioCapabilities::supported_channel(info->channel,info->isInput==ASIOTrue))
      return ASE_InvalidParameter;
    info->isActive=ASIOFalse; info->channelGroup=0; info->type=ASIOSTFloat32LSB;
    std::strcpy(info->name,info->isInput==ASIOTrue ? "SR850 Guitar Input" : "SR850 Corrected Output");
    return ASE_OK;
  }
  ASIOError createBuffers(ASIOBufferInfo*,long,long,ASIOCallbacks*) override {
    return ASE_NotPresent; // No virtual audio callback/physical bridge yet.
  }
  ASIOError disposeBuffers() override { stream_.close(); return ASE_OK; }
  ASIOError controlPanel() override { return ASE_NotPresent; }
  ASIOError future(long,void*) override { return ASE_NotPresent; }
  ASIOError outputReady() override { return ASE_NotPresent; }
 private:
  VirtualAsioStreamCore stream_;
  ASIOSampleRate sample_rate_=48000.0;
};
} // namespace sr850
#endif
