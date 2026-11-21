#include "asio_duplex_callback_router.hpp"
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>
int main() {
  sr850::AsioDuplexCallbackRouter router;
  router.configure(std::vector<float>{1.0f},1,2,4);
  float guitar[4]={0.125f,-0.25f,0.5f,-0.75f};
  float guitar_copy[4]={};
  float left[4]={1.0f,0.5f,-0.5f,0.25f};
  float right[4]={-1.0f,0.25f,0.5f,-0.25f};
  float out_left[4]={},out_right[4]={};
  const float* in[1]={guitar};
  float* host_in[1]={guitar_copy};
  const float* playback[2]={left,right};
  float* device_out[2]={out_left,out_right};
  assert(router.process(0,in,host_in,playback,device_out,4,0.5f,true));
  assert(std::memcmp(guitar,guitar_copy,sizeof(guitar))==0);
  for(int i=0;i<4;++i) {
    assert(std::fabs(out_left[i]-left[i]*0.5f)<0.00001f);
    assert(std::fabs(out_right[i]-right[i]*0.5f)<0.00001f);
  }
  assert(router.process(1,in,host_in,playback,device_out,4,1.0f,false));
  assert(std::memcmp(guitar,guitar_copy,sizeof(guitar))==0);
  assert(!router.process(2,in,host_in,playback,device_out,4,1.0f,true));
  std::cout<<"ASIO duplex callback router tests passed\\n";
}
