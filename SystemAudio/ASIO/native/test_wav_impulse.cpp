#include "wav_impulse.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <vector>
static void write_wav(const char* path, unsigned rate, unsigned channels,
                      const std::vector<unsigned char>& data) {
  std::ofstream f(path, std::ios::binary);
  auto u16=[&](unsigned v){ f.put(char(v)); f.put(char(v>>8)); };
  auto u32=[&](unsigned v){ u16(v & 65535); u16(v >> 16); };
  f.write("RIFF",4); u32(unsigned(36+data.size())); f.write("WAVEfmt ",8);
  u32(16); u16(1); u16(channels); u32(rate); u32(rate*channels*2);
  u16(channels*2); u16(16); f.write("data",4);
  u32(unsigned(data.size()));
  f.write(reinterpret_cast<const char*>(data.data()),std::streamsize(data.size()));
}
int main() {
  const char* path="sr850_wav_impulse_test.wav";
  write_wav(path,48000,1,{0,0,0,64,0,192});
  const auto wav=sr850::load_mono_wav_impulse(path,48000);
  assert(wav.sample_rate==48000 && wav.taps.size()==3);
  assert(std::fabs(wav.taps[0])<0.0001f);
  assert(std::fabs(wav.taps[1]-0.5f)<0.0001f);
  assert(std::fabs(wav.taps[2]+0.5f)<0.0001f);
  bool rejected=false;
  try { (void)sr850::load_mono_wav_impulse(path,44100); }
  catch (const std::runtime_error&) { rejected=true; }
  assert(rejected);
  write_wav(path,48000,2,{0,0,0,0});
  rejected=false;
  try { (void)sr850::load_mono_wav_impulse(path,48000); }
  catch (const std::runtime_error&) { rejected=true; }
  assert(rejected);
  std::remove(path);
}
