#include "device_session.hpp"
#include <cassert>
#include <iostream>
int main() {
  using namespace sr850;
  DeviceSession session;
  const StreamFormat app{48000,1,2,128,SampleType::float32};
  const StreamFormat hw{48000,1,2,128,SampleType::pcm16};
  assert(session.state()==SessionState::closed);
  assert(session.start()==SessionError::invalid_state);
  assert(session.configure(app,hw,256)==SessionError::ok);
  assert(session.configure(app,hw,256)==SessionError::invalid_state);
  assert(session.state()==SessionState::configured);
  assert(session.start()==SessionError::ok);
  assert(session.start()==SessionError::invalid_state);
  assert(session.stop()==SessionError::ok);
  assert(session.stop()==SessionError::invalid_state);
  session.close();
  assert(session.state()==SessionState::closed);
  auto bad=hw;
  bad.sample_rate=44100;
  assert(session.configure(app,bad,256)==SessionError::unsupported_stream);
  assert(session.state()==SessionState::closed);
  std::cout<<"Device session state tests passed\n";
}
