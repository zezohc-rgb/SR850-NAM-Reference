#include "format_preflight.hpp"
#include <cassert>
int main() {
  using namespace sr850;
  const StreamFormat app{48000,2,2,256,SampleType::float32};
  StreamFormat hardware{48000,2,2,256,SampleType::pcm16};
  assert(negotiate_with_conversion(app,hardware,1024,{})==
         NegotiationResult::format_conversion_required);
  assert(negotiate_with_conversion(app,hardware,1024,{true,false,false,false})==
         NegotiationResult::format_conversion_required);
  assert(negotiate_with_conversion(app,hardware,1024,{true,true,false,false})==
         NegotiationResult::ok);
  hardware.sample_type=SampleType::pcm32;
  assert(negotiate_with_conversion(app,hardware,1024,{true,true,false,false})==
         NegotiationResult::format_conversion_required);
  assert(negotiate_with_conversion(app,hardware,1024,{false,false,true,true})==
         NegotiationResult::ok);
  hardware.sample_type=SampleType::float32;
  assert(negotiate_with_conversion(app,hardware,1024,{})==NegotiationResult::ok);
  hardware.sample_rate=44100;
  assert(negotiate_with_conversion(app,hardware,1024,{})==
         NegotiationResult::unsupported_rate);
}
