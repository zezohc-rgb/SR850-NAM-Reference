#include "asio_double_buffer_bridge.hpp"
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>
int main() {
  sr850::AsioDoubleBufferBridge bridge;
  bridge.configure(std::vector<float>{1.0f},1,2,4);
  float guitar[2][4]={{0.25f,-0.5f,0.75f,0.0f},{-0.25f,0.5f,-0.75f,0.0f}};
  float input_copy[2][4]={{},{}}; 
  float left[2][4]={{1,0.5f,0,-0.5f},{-1,-0.5f,0,0.5f}};
  float right[2][4]={{-1,-0.5f,0,0.5f},{1,0.5f,0,-0.5f}};
  float output_left[2][4]={{},{}}; float output_right[2][4]={{},{}};
  assert(!bridge.process(0,4,1.0f,true));
  for(unsigned bank=0;bank<2;++bank) {
    const float* di[1]={guitar[bank]}; float* hi[1]={input_copy[bank]};
    const float* ho[2]={left[bank],right[bank]};
    float* dout[2]={output_left[bank],output_right[bank]};
    assert(bridge.bind(bank,di,hi,ho,dout));
  }
  for(unsigned bank=0;bank<2;++bank) {
    assert(bridge.process(bank,4,0.5f,true));
    assert(std::memcmp(guitar[bank],input_copy[bank],sizeof(guitar[bank]))==0);
    for(int i=0;i<4;++i) {
      assert(std::fabs(output_left[bank][i]-left[bank][i]*0.5f)<0.00001f);
      assert(std::fabs(output_right[bank][i]-right[bank][i]*0.5f)<0.00001f);
    }
  }
  assert(!bridge.process(2,4,1.0f,true));
  std::cout<<"ASIO double-buffer bridge tests passed\\n";
}
