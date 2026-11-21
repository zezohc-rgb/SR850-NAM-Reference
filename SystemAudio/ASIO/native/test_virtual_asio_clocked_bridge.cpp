#include "virtual_asio_clocked_bridge.hpp"
#include <cassert>
#include <cmath>
struct Context { sr850::VirtualAsioStreamCore* stream; int callbacks=0; };
void host(void* raw,std::size_t bank) {
 auto& ctx=*static_cast<Context*>(raw);
 assert(bank==static_cast<std::size_t>(ctx.callbacks%2));
 auto* guitar=ctx.stream->input(bank,0);
 ctx.stream->output(bank,0)[0]=guitar[0];
 ctx.stream->output(bank,1)[0]=guitar[0]*0.25f;
 ++ctx.callbacks;
}
int main() {
 sr850::VirtualAsioStreamCore stream;
 assert(stream.prepare(128,2,2));
 sr850::VirtualAsioClockedBridge bridge(stream);
 Context ctx{&stream,0};
 assert(!bridge.physical_buffer_ready());
 assert(bridge.prepare(stream,{0.5f},48000,host,&ctx));
 assert(bridge.start());
 stream.input(0,0)[0]=0.8f;
 stream.input(1,0)[0]=0.4f;
 assert(bridge.physical_buffer_ready());
 assert(bridge.physical_buffer_ready());
 assert(ctx.callbacks==2);
 assert(bridge.sample_position()==256);
 assert(bridge.completed_blocks()==2);
 assert(std::fabs(stream.input(0,0)[0]-0.8f)<1e-6f);
 assert(std::fabs(stream.input(1,0)[0]-0.4f)<1e-6f);
 assert(std::fabs(stream.output(0,0)[0]-0.4f)<1e-6f);
 assert(std::fabs(stream.output(0,1)[0]-0.1f)<1e-6f);
 assert(std::fabs(stream.output(1,0)[0]-0.2f)<1e-6f);
 assert(std::fabs(stream.output(1,1)[0]-0.05f)<1e-6f);
 bridge.stop();
 assert(!bridge.physical_buffer_ready());
 stream.close();
}
