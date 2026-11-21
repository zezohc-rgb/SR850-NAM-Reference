#pragma once
// Coordinates prepared physical ASIO buffers with SR850 FIR routing.
// Does not create a virtual ASIO device or initialize hardware.
#ifdef _WIN32
#include "physical_asio_stream_windows.hpp"
#include "asio_sdk_buffer_binder_windows.hpp"
#include <cstddef>

namespace sr850 {
class PhysicalAsioBridgeSession {
 public:
  PhysicalAsioBridgeSession(PhysicalAsioStream& stream,
                            AsioDoubleBufferBridge& bridge) noexcept
      : stream_(stream), bridge_(bridge) {}
  // PhysicalAsioStream::prepare must have succeeded. Host buffer arrays
  // must remain valid until stream.stop() and stream.shutdown() complete.
  bool bind_prepared_buffers(std::size_t input_channels,
                             std::size_t output_channels,
                             const float* const* host_playback_banks[2],
                             float* const* host_input_banks[2]) {
    const auto& buffers=stream_.buffers();
    if (!input_channels || !output_channels ||
        input_channels>64 || output_channels>64 ||
        buffers.size()!=input_channels+output_channels) return false;
    if (!binder_.configure(bridge_,buffers.data(),input_channels,
                           buffers.data()+input_channels,output_channels,
                           host_playback_banks,host_input_banks)) return false;
    return stream_.mark_buffers_bound();
  }
 private:
  PhysicalAsioStream& stream_;
  AsioDoubleBufferBridge& bridge_;
  AsioSdkBufferBinder binder_;
};
} // namespace sr850
#endif
