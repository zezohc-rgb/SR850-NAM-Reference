#include "wav_impulse.hpp"
#include "fir_processor.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <vector>
int main() {
  const char* filename="sr850_integration_impulse.wav";
  {
    std::ofstream f(filename,std::ios::binary);
    auto u16=[&](unsigned v){f.put(char(v));f.put(char(v>>8));};
    auto u32=[&](unsigned v){u16(v&65535);u16(v>>16);};
    f.write("RIFF",4);u32(40);f.write("WAVEfmt ",8);
    u32(16);u16(1);u16(1);u32(48000);u32(96000);u16(2);u16(16);
    f.write("data",4);u32(4);u16(16384);u16(16384);
  }
  const auto impulse=sr850::load_mono_wav_impulse(filename,48000);
  sr850::FirProcessor fir;
  fir.configure(impulse.taps,2);
  float samples[]={1.0f,0.0f,0.0f,1.0f,0.0f,0.0f};
  fir.process(samples,3,1.0f,true);
  // Independent L/R histories: a two-tap moving average at 0.5 gain per tap.
  assert(std::fabs(samples[0]-0.5f)<0.0001f);
  assert(std::fabs(samples[1])<0.0001f);
  assert(std::fabs(samples[2]-0.5f)<0.0001f);
  assert(std::fabs(samples[3]-0.5f)<0.0001f);
  assert(std::fabs(samples[4])<0.0001f);
  assert(std::fabs(samples[5]-0.5f)<0.0001f);
  fir.reset();
  float bypass[]={0.25f,-0.75f};
  fir.process(bypass,1,1.0f,false);
  assert(bypass[0]==0.25f && bypass[1]==-0.75f);
  std::remove(filename);
}
