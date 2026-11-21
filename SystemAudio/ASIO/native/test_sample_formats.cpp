#include "sample_formats.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <iostream>
int main() {
  assert(sr850::pcm16_to_float(-32768)==-1.0f);
  assert(sr850::float_to_pcm16(1.0f)==32767);
  assert(sr850::float_to_pcm16(-1.0f)==-32768);
  assert(sr850::float_to_pcm16(3.0f)==32767);
  assert(sr850::float_to_pcm16(std::numeric_limits<float>::quiet_NaN())==0);
  assert(sr850::pcm32_to_float(INT32_MIN)==-1.0f);
  assert(sr850::float_to_pcm32(1.0f)==INT32_MAX);
  assert(sr850::float_to_pcm32(-1.0f)==INT32_MIN);
  const std::int16_t left[]={-32768,0,32767};
  const std::int16_t right[]={16384,-16384,0};
  const std::int16_t* source[]={left,right};
  float out_left[3]={},out_right[3]={};
  float* out[]={out_left,out_right};
  assert(sr850::planar_pcm16_to_float(source,out,2,3));
  assert(out_left[0]==-1.f && out_right[0]==.5f);
  const float* floats[]={out_left,out_right};
  std::int16_t dst_left[3]={},dst_right[3]={};
  std::int16_t* dest[]={dst_left,dst_right};
  assert(sr850::planar_float_to_pcm16(floats,dest,2,3));
  assert(dst_left[0]==left[0] && dst_right[1]==right[1]);
  assert(!sr850::planar_pcm16_to_float(nullptr,out,2,3));
  std::cout<<"PCM conversion tests passed\n";
}
