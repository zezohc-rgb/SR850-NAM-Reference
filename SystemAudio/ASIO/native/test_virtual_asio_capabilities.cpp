#include "virtual_asio_capabilities.hpp"
#include <cassert>
int main() {
 using C=sr850::VirtualAsioCapabilities;
 static_assert(C::input_channels==2 && C::output_channels==2,"stereo");
 static_assert(C::sample_type_float32_lsb==19,"ASIO float32 little endian");
 assert(C::supported_rate(44100.0));
 assert(C::supported_rate(48000.0));
 assert(!C::supported_rate(96000.0));
 assert(C::supported_frames(64));
 assert(C::supported_frames(256));
 assert(C::supported_frames(1024));
 assert(!C::supported_frames(128+64));
 assert(!C::supported_frames(2048));
 assert(C::supported_channel(1,true));
 assert(C::supported_channel(1,false));
 assert(!C::supported_channel(2,true));
 assert(!C::supported_channel(-1,false));
}
