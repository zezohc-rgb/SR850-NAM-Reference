#include "virtual_asio_sample_clock.hpp"
#include <cassert>
#include <cmath>
int main() {
 sr850::VirtualAsioSampleClock clock;
 assert(!clock.advance());
 assert(!clock.configure(0,48000));
 assert(!clock.configure(256,96000));
 assert(clock.configure(256,48000));
 assert(clock.position()==0);
 for(int i=0;i<188;++i) assert(clock.advance());
 assert(clock.position()==48128);
 assert(clock.blocks()==188);
 assert(std::fabs(clock.seconds()-48128.0/48000.0)<1e-12);
 clock.reset();
 assert(clock.position()==0 && clock.blocks()==0);
 assert(clock.configure(128,44100));
 assert(clock.advance());
 assert(clock.position()==128 && clock.sample_rate()==44100);
}
