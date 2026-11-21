#include "preflight_backend_controller.hpp"
#include <cassert>
struct Source : sr850::AsioCapabilitySource {
  sr850::AsioCapabilities caps{2,2,64,1024,256,-1,48000};
  bool query(sr850::AsioCapabilities& out) override {out=caps;return true;}
};
struct Backend : sr850::AsioBackend {
  bool opened=false, prepared=false, started=false;
  int closes=0;
  sr850::StreamFormat device{48000,2,2,256,sr850::SampleType::pcm16};
  bool open() override {opened=true;return true;}
  bool query_stream(sr850::StreamFormat& out) override {out=device;return true;}
  bool prepare(const sr850::StreamFormat&) override {prepared=true;return true;}
  bool start() override {started=true;return true;}
  bool stop() override {started=false;return true;}
  void close() override {opened=false;++closes;}
};
int main() {
  using namespace sr850;
  Backend backend;
  Source source;
  const StreamFormat app{48000,2,2,256,SampleType::float32};
  {
    PreflightBackendController controller(backend,source);
    assert(!controller.prepare(app,1024));
    assert(!backend.prepared);
  }
  {
    PreflightBackendController controller(backend,source,ConversionSupport{true,true,false,false});
    assert(controller.prepare(app,1024));
    assert(backend.prepared);
    assert(controller.start() && controller.running());
    assert(controller.stop() && !controller.running());
  }
  assert(!backend.opened && backend.closes==1);
  backend.prepared=false;
  source.caps.sample_rate=44100;
  {
    PreflightBackendController controller(backend,source);
    assert(!controller.prepare(app,1024));
    assert(!backend.prepared && !backend.opened);
  }
  source.caps.sample_rate=48000;
  backend.device.buffer_frames=512;
  {
    PreflightBackendController controller(backend,source);
    assert(!controller.prepare(app,1024));
    assert(!backend.prepared && !backend.opened);
  }
}
