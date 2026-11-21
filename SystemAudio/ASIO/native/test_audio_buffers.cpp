#include "audio_buffers.hpp"
#include "fir_processor.hpp"
#include <cassert>
#include <iostream>
int main() {
  const float left[]={1,2,3};
  const float right[]={4,5,6};
  const float* input[]={left,right};
  float interleaved[6]={};
  assert(sr850::planar_to_interleaved(input,interleaved,2,3));
  assert(interleaved[0]==1 && interleaved[1]==4 && interleaved[4]==3);
  sr850::FirProcessor fir;
  fir.configure({1.0f},2);
  fir.process(interleaved,3,1.0f,true);
  float output_left[3]={},output_right[3]={};
  float* output[]={output_left,output_right};
  assert(sr850::interleaved_to_planar(interleaved,output,2,3));
  assert(output_left[2]==3 && output_right[2]==6);
  float copied_left[3]={},copied_right[3]={};
  float* copied[]={copied_left,copied_right};
  assert(sr850::copy_input_channels(input,copied,2,3));
  for(int i=0;i<3;++i) {
    assert(copied_left[i]==left[i]);
    assert(copied_right[i]==right[i]);
  }
  const float* invalid[]={left,nullptr};
  assert(!sr850::planar_to_interleaved(invalid,interleaved,2,3));
  std::cout<<"ASIO-style buffer conversion tests passed\n";
}
