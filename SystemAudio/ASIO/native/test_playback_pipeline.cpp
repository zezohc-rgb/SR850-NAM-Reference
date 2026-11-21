#include "playback_pipeline.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main() {
  sr850::PlaybackPipeline pipeline;
  pipeline.configure({0.5f,0.5f},2,4);
  float in_left[]={1,0,0,0},in_right[]={0,1,0,0};
  const float* input[]={in_left,in_right};
  float out_left[4]={},out_right[4]={};
  float* output[]={out_left,out_right};
  assert(pipeline.process(input,output,4,1.0f,true));
  assert(std::fabs(out_left[0]-.5f)<1e-6f);
  assert(std::fabs(out_left[1]-.5f)<1e-6f);
  assert(std::fabs(out_right[1]-.5f)<1e-6f);
  assert(std::fabs(out_right[2]-.5f)<1e-6f);
  pipeline.reset();
  assert(pipeline.process(input,output,4,1.0f,false));
  assert(out_left[0]==1 && out_left[1]==0);
  assert(out_right[0]==0 && out_right[1]==1);
  assert(!pipeline.process(input,output,5,1.0f,true));
  std::cout<<"Playback pipeline tests passed\n";
}
