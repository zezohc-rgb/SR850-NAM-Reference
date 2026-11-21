#include "asio_preflight.hpp"
#include <cassert>
#include <limits>
struct Mock : sr850::AsioCapabilitySource {
  bool available=true;
  int calls=0;
  sr850::AsioCapabilities data{2,2,64,1024,256,-1,48000};
  bool query(sr850::AsioCapabilities& out) override {
    ++calls;
    if (!available) return false;
    out=data;return true;
  }
};
int main() {
  using namespace sr850;
  Mock mock;
  AsioCapabilities output{};
  StreamFormat request{48000,1,2,256,SampleType::float32};
  assert(preflight_asio_stream(mock,request,1024,output)==AsioPreflightError::ok);
  assert(output.output_channels==2);
  request.sample_rate=44100;
  assert(preflight_asio_stream(mock,request,1024,output)==AsioPreflightError::unsupported_configuration);
  request.sample_rate=48000;
  request.buffer_frames=96;
  assert(preflight_asio_stream(mock,request,1024,output)==AsioPreflightError::unsupported_configuration);
  request.buffer_frames=256;
  mock.available=false;
  assert(preflight_asio_stream(mock,request,1024,output)==AsioPreflightError::query_failed);
  mock.available=true;
  mock.data.min_buffer_frames=0;
  assert(preflight_asio_stream(mock,request,1024,output)==AsioPreflightError::invalid_capabilities);
  mock.data.min_buffer_frames=64;
  const int before=mock.calls;
  request.buffer_frames=2048;
  assert(preflight_asio_stream(mock,request,1024,output)==AsioPreflightError::invalid_requested_format);
  assert(mock.calls==before);
  request.buffer_frames=256;
  request.input_channels=(std::numeric_limits<std::size_t>::max)();
  assert(preflight_asio_stream(mock,request,1024,output)==AsioPreflightError::invalid_requested_format);
}
