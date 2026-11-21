#include "asio_backend.hpp"
#include <cassert>
#include <iostream>
struct MockBackend final : sr850::AsioBackend {
  bool fail_prepare=false, fail_start=false;
  int opened=0, prepared=0, started=0, stopped=0, closed=0;
  bool open() override { ++opened; return true; }
  bool query_stream(sr850::StreamFormat& format) override {
    format={48000,1,2,128,sr850::SampleType::pcm16}; return true;
  }
  bool prepare(const sr850::StreamFormat&) override { ++prepared; return !fail_prepare; }
  bool start() override { ++started; return !fail_start; }
  bool stop() override { ++stopped; return true; }
  void close() override { ++closed; }
};
int main() {
  using namespace sr850;
  MockBackend backend;
  BackendController controller(backend);
  const StreamFormat app{48000,1,2,128,SampleType::float32};
  assert(!controller.start());
  assert(controller.open_and_prepare(app,512));
  assert(!controller.open_and_prepare(app,512));
  assert(controller.start());
  assert(controller.running());
  assert(controller.stop());
  assert(!controller.running());
  controller.close();
  assert(backend.closed==1);
  backend.fail_prepare=true;
  assert(!controller.open_and_prepare(app,512));
  assert(backend.closed==2);
  backend.fail_prepare=false;
  backend.fail_start=true;
  assert(controller.open_and_prepare(app,512));
  assert(!controller.start());
  assert(!controller.running());
  controller.close();
  assert(backend.closed==3);
  std::cout<<"Backend lifecycle mock tests passed\n";
}
