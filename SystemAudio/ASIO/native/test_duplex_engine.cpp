#include "duplex_engine.hpp"
#include <cassert>
#include <iostream>
int main() {
  sr850::DuplexEngine engine;
  engine.configure({1.0f},2,4);
  float guitar[]={.25f,.5f,.75f,1.f};
  const float* hardware_in[]={guitar};
  float app_guitar[4]={};
  float* app_in[]={app_guitar};
  float left[]={1,2,3,4},right[]={5,6,7,8};
  const float* app_out[]={left,right};
  float hw_left[4]={},hw_right[4]={};
  float* hardware_out[]={hw_left,hw_right};
  assert(engine.process(hardware_in,app_in,1,app_out,hardware_out,4,.5f,true));
  assert(app_guitar[0]==guitar[0] && app_guitar[3]==guitar[3]);
  assert(hw_left[0]==.5f && hw_right[3]==4.f);
  app_guitar[0]=99.f;
  assert(!engine.process(hardware_in,app_in,1,app_out,hardware_out,5,1.f,true));
  assert(app_guitar[0]==99.f);
  engine.reset();
  assert(engine.process(hardware_in,app_in,1,app_out,hardware_out,4,1.f,false));
  assert(hw_left[0]==left[0] && hw_right[3]==right[3]);
  std::cout<<"Duplex routing tests passed\n";
}
