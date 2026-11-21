#include "fir_processor.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
static bool near(float a, float b) { return std::fabs(a-b)<1e-5f; }
int main() {
  sr850::FirProcessor p;
  p.configure({0.5f, 0.5f}, 2);
  float stereo[]={1,0, 0,1, 0,0};
  p.process(stereo,3,1.0f,true);
  assert(near(stereo[0],.5f)&&near(stereo[1],0));
  assert(near(stereo[2],.5f)&&near(stereo[3],.5f));
  assert(near(stereo[4],0)&&near(stereo[5],.5f));
  p.reset();
  float bypass[]={1,2, 3,4};
  p.process(bypass,2,1.0f,false);
  assert(near(bypass[0],1)&&near(bypass[3],4));
  p.reset();
  float gain[]={1,1};
  p.process(gain,1,2.0f,true);
  assert(near(gain[0],1)&&near(gain[1],1));
  std::cout<<"FIR core tests passed\n";
}
