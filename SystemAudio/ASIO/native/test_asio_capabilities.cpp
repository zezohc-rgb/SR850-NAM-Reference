#include "asio_capabilities.hpp"
#include <cassert>
int main() {
  using namespace sr850;
  AsioCapabilities c{2,2,64,1024,256,-1,48000.0};
  assert(validate_asio_capabilities(c,2,2,256)==CapabilityError::ok);
  assert(validate_asio_capabilities(c,2,2,128)==CapabilityError::ok);
  assert(validate_asio_capabilities(c,2,2,96)==CapabilityError::unsupported_buffer_size);
  assert(validate_asio_capabilities(c,3,2,256)==CapabilityError::unsupported_channel_count);
  assert(validate_asio_capabilities(c,2,3,256)==CapabilityError::unsupported_channel_count);
  assert(validate_asio_capabilities(c,2,2,2048)==CapabilityError::unsupported_buffer_size);
  c.buffer_granularity=0;c.min_buffer_frames=256;c.max_buffer_frames=256;
  assert(validate_asio_capabilities(c,2,2,256)==CapabilityError::ok);
  assert(validate_asio_capabilities(c,2,2,128)==CapabilityError::unsupported_buffer_size);
  c.buffer_granularity=32;c.min_buffer_frames=64;c.max_buffer_frames=1024;
  assert(validate_asio_capabilities(c,2,2,96)==CapabilityError::ok);
  assert(validate_asio_capabilities(c,2,2,100)==CapabilityError::unsupported_buffer_size);
  c.sample_rate=96000;
  assert(validate_asio_capabilities(c,2,2,256)==CapabilityError::unsupported_rate);
  c.sample_rate=48000;c.preferred_buffer_frames=63;
  assert(validate_asio_capabilities(c,2,2,256)==CapabilityError::invalid_buffers);
}
