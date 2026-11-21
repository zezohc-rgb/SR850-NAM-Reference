#include "virtual_asio_physical_transport.hpp"
#include <cassert>
#include <cmath>
struct HostState { sr850::VirtualAsioStreamCore* stream; int callbacks=0; };
void host(void* raw,std::size_t bank) {
 auto& ctx=*static_cast<HostState*>(raw);
 const float guitar=ctx.stream->input(bank,0)[0];
 const float second=ctx.stream->input(bank,1)[0];
 ctx.stream->output(bank,0)[0]=guitar;
 ctx.stream->output(bank,1)[0]=second;
 ++ctx.callbacks;
}
int main() {
 sr850::VirtualAsioStreamCore stream;
 assert(stream.prepare(64,2,2));
 sr850::VirtualAsioPhysicalTransport transport(stream);
 HostState state{&stream,0};
 assert(transport.prepare({0.5f},48000,host,&state));
 assert(transport.start());
 float in0[64]={},in1[64]={},out0[64]={},out1[64]={};
 const float* ins[2]={in0,in1};
 float* outs[2]={out0,out1};
 in0[0]=0.8f;in1[0]=0.4f;
 assert(transport.process(ins,outs));
 assert(std::fabs(out0[0]-0.4f)<1e-6f);
 assert(std::fabs(out1[0]-0.2f)<1e-6f);
 assert(std::fabs(in0[0]-0.8f)<1e-6f);
 assert(std::fabs(stream.input(0,0)[0]-0.8f)<1e-6f);
 in0[0]=0.6f;in1[0]=0.2f;
 assert(transport.process(ins,outs));
 assert(std::fabs(out0[0]-0.3f)<1e-6f);
 assert(std::fabs(out1[0]-0.1f)<1e-6f);
 assert(state.callbacks==2 && transport.completed_blocks()==2);
 transport.stop();
 assert(!transport.process(ins,outs));
 stream.close();
}
