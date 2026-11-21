#include "stream_negotiation.hpp"
#include <cassert>
#include <iostream>
int main() {
  using namespace sr850;
  const StreamFormat app{44100,1,2,128,SampleType::float32};
  const StreamFormat hardware{44100,1,2,128,SampleType::pcm16};
  assert(negotiate_direct(app,hardware,512)==NegotiationResult::ok);
  auto mismatch=hardware;
  mismatch.sample_rate=48000;
  assert(negotiate_direct(app,mismatch,512)==NegotiationResult::unsupported_rate);
  mismatch=hardware;
  mismatch.output_channels=1;
  assert(negotiate_direct(app,mismatch,512)==NegotiationResult::channel_mismatch);
  mismatch=hardware;
  mismatch.buffer_frames=256;
  assert(negotiate_direct(app,mismatch,512)==NegotiationResult::buffer_mismatch);
  mismatch=hardware;
  mismatch.sample_type=SampleType::unsupported;
  assert(negotiate_direct(app,mismatch,512)==NegotiationResult::unsupported_format);
  assert(negotiate_direct(app,hardware,64)==NegotiationResult::buffer_mismatch);
  mismatch=hardware;
  mismatch.sample_rate=0;
  assert(negotiate_direct(app,mismatch,512)==NegotiationResult::invalid_configuration);
  std::cout<<"Stream negotiation tests passed\n";
}
