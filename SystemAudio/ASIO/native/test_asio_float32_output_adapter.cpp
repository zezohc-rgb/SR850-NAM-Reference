#include "asio_float32_output_adapter.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
int main() {
  sr850::AsioFloat32OutputAdapter adapter;
  adapter.configure(std::vector<float>{1.0f}, 2, 4);
  float left[4]={0.25f,0.5f,-0.5f,0.0f};
  float right[4]={-0.25f,0.25f,0.5f,0.0f};
  float out_left[4]={}, out_right[4]={};
  const float* input[2]={left,right};
  float* output[2]={out_left,out_right};
  assert(adapter.process(0,input,output,4,1.0f,true));
  for(int i=0;i<4;++i) {
    assert(std::fabs(out_left[i]-left[i])<0.00001f);
    assert(std::fabs(out_right[i]-right[i])<0.00001f);
  }
  assert(adapter.process(1,input,output,4,0.5f,true));
  for(int i=0;i<4;++i) {
    assert(std::fabs(out_left[i]-left[i]*0.5f)<0.00001f);
    assert(std::fabs(out_right[i]-right[i]*0.5f)<0.00001f);
  }
  assert(!adapter.process(2,input,output,4,1.0f,true));
  assert(!adapter.process(0,input,output,5,1.0f,true));
  std::cout<<"ASIO float32 output adapter tests passed\\n";
}
