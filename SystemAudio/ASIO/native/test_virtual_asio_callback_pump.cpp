#include "virtual_asio_callback_pump.hpp"
#include <cassert>
struct Context {
 sr850::VirtualAsioStreamCore* stream;
 int calls=0;
};
void callback(void* raw,std::size_t bank) {
 auto& ctx=*static_cast<Context*>(raw);
 assert(bank==static_cast<std::size_t>(ctx.calls%2));
 float* input=ctx.stream->input(bank,0);
 float* output=ctx.stream->output(bank,0);
 assert(input && output);
 output[0]=input[0]*0.5f;
 ++ctx.calls;
}
int main() {
 sr850::VirtualAsioStreamCore stream;
 sr850::VirtualAsioCallbackPump pump(stream);
 Context ctx{&stream,0};
 assert(!pump.start());
 assert(!pump.tick());
 assert(stream.prepare(128,2,2));
 assert(!pump.attach(nullptr,&ctx));
 assert(pump.attach(callback,&ctx));
 assert(pump.configure_clock(48000));
 assert(pump.start());
 stream.input(0,0)[0]=0.8f;
 stream.input(1,0)[0]=0.6f;
 assert(pump.tick());
 assert(pump.tick());
 assert(ctx.calls==2);
 assert(pump.sample_position()==256);
 assert(pump.completed_blocks()==2);
 assert(stream.output(0,0)[0]>0.39f && stream.output(0,0)[0]<0.41f);
 assert(stream.output(1,0)[0]>0.29f && stream.output(1,0)[0]<0.31f);
 pump.stop();
 assert(!pump.tick());
 stream.close();
}
