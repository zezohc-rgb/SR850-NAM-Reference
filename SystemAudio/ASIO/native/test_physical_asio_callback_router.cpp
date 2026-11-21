#include "physical_asio_callback_router.hpp"
#include <cassert>
#include <cmath>
struct Context { sr850::VirtualAsioStreamCore* stream; };
void host(void* raw,std::size_t bank) {
 auto& ctx=*static_cast<Context*>(raw);
 for(std::size_t ch=0;ch<2;++ch)
   ctx.stream->output(bank,ch)[0]=ctx.stream->input(bank,ch)[0];
}
int main() {
 sr850::VirtualAsioStreamCore stream;
 assert(stream.prepare(64,2,2));
 sr850::VirtualAsioPhysicalTransport transport(stream);
 sr850::PhysicalAsioCallbackRouter router(transport);
 assert(!router.configure_frames(0));
 assert(router.configure_frames(64));
 Context context{&stream};
 float in[2][2][64]={},out[2][2][64]={};
 for(std::size_t bank=0;bank<2;++bank) {
   const float* inputs[2]={in[bank][0],in[bank][1]};
   float* outputs[2]={out[bank][0],out[bank][1]};
   assert(router.bind(bank,inputs,outputs));
 }
 assert(transport.prepare({0.5f},48000,host,&context));
 assert(transport.start());
 in[0][0][0]=0.8f;in[0][1][0]=0.4f;
 in[1][0][0]=0.6f;in[1][1][0]=0.2f;
 assert(!router.buffer_switch(-1));
 assert(!router.buffer_switch(2));
 out[1][0][0]=0.9f;
 assert(!router.buffer_switch(1)); // unexpected bank order
 assert(out[1][0][0]==0.0f); // stale playback must be silenced
 assert(router.buffer_switch(0));
 assert(router.buffer_switch(1));
 assert(std::fabs(out[0][0][0]-0.4f)<1e-6f);
 assert(std::fabs(out[0][1][0]-0.2f)<1e-6f);
 assert(std::fabs(out[1][0][0]-0.3f)<1e-6f);
 assert(std::fabs(out[1][1][0]-0.1f)<1e-6f);
 assert(transport.completed_blocks()==2);
 transport.stop();
 assert(!router.buffer_switch(0));
}
