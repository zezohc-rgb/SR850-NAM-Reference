#include "virtual_asio_stream_core.hpp"
#include <cassert>
int main() {
  sr850::VirtualAsioStreamCore core;
  assert(!core.start());
  assert(!core.prepare(0,1,2));
  assert(core.prepare(128,1,2));
  assert(!core.prepare(128,1,2));
  assert(core.input(0,0) && core.input(1,0));
  assert(core.output(0,0) && core.output(1,1));
  assert(core.output(0,2)==nullptr);
  assert(core.input(2,0)==nullptr);
  core.output(0,0)[0]=0.25f;
  assert(core.output(1,0)[0]==0.0f);
  assert(core.start());
  assert(!core.start());
  core.stop();
  assert(core.state()==sr850::VirtualAsioStreamCore::State::prepared);
  core.close();
  assert(core.output(0,0)==nullptr);
  assert(core.prepare(256,2,2));
  core.close();
}
