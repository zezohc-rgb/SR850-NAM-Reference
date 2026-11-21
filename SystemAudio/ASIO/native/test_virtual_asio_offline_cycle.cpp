#include "virtual_asio_offline_cycle.hpp"
#include "virtual_asio_callback_pump.hpp"
#include <cassert>
#include <cmath>
struct TestContext {
 sr850::VirtualAsioStreamCore* stream;
 sr850::VirtualAsioOfflineCycle* cycle;
 int calls=0;
};
void host(void* raw,std::size_t bank) {
 auto& ctx=*static_cast<TestContext*>(raw);
 const auto* guitar=ctx.stream->input(bank,0);
 auto* left=ctx.stream->output(bank,0);
 auto* right=ctx.stream->output(bank,1);
 left[0]=guitar[0]; right[0]=guitar[0]*0.5f;
 ++ctx.calls;
}
void tick(void* raw,std::size_t bank) {
 auto& ctx=*static_cast<TestContext*>(raw);
 assert(ctx.cycle->process(bank,host,&ctx,1.0f,true));
}
int main() {
 sr850::VirtualAsioStreamCore stream;
 assert(stream.prepare(64,2,2));
 sr850::VirtualAsioOfflineCycle cycle;
 assert(cycle.configure(stream,{0.5f}));
 TestContext ctx{&stream,&cycle,0};
 sr850::VirtualAsioCallbackPump pump(stream);
 assert(pump.attach(tick,&ctx));
 assert(pump.configure_clock(48000));
 assert(pump.start());
 stream.input(0,0)[0]=0.8f;
 stream.input(1,0)[0]=0.6f;
 assert(pump.tick() && pump.tick());
 assert(ctx.calls==2);
 assert(pump.sample_position()==128);
 assert(std::fabs(stream.input(0,0)[0]-0.8f)<1e-6f);
 assert(std::fabs(stream.input(1,0)[0]-0.6f)<1e-6f);
 assert(std::fabs(stream.output(0,0)[0]-0.4f)<1e-6f);
 assert(std::fabs(stream.output(0,1)[0]-0.2f)<1e-6f);
 assert(std::fabs(stream.output(1,0)[0]-0.3f)<1e-6f);
 assert(std::fabs(stream.output(1,1)[0]-0.15f)<1e-6f);
 pump.stop();
 assert(!pump.tick());
 stream.close();
}
