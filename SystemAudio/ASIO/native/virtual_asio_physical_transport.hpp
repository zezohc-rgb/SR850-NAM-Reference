#pragma once
// Offline transport adapter for a future physical ASIO callback.
// Only float32 planar channels; caller must validate physical driver sample type.
// The physical callback provides pointers valid for the entire synchronous call.
// All configuration happens with audio stopped; process() allocates nothing.
#include "virtual_asio_clocked_bridge.hpp"
#include <cstddef>
#include <cstring>
namespace sr850 {
class VirtualAsioPhysicalTransport {
 public:
  explicit VirtualAsioPhysicalTransport(VirtualAsioStreamCore& stream)
    : stream_(stream),bridge_(stream) {}
  bool prepare(const std::vector<float>& impulse,std::uint32_t rate,
               VirtualAsioClockedBridge::HostCallback host,void* context) {
    return bridge_.prepare(stream_,impulse,rate,host,context);
  }
  bool start() noexcept { return bridge_.start(); }
  std::size_t frames() const noexcept { return stream_.frames(); }
  // Physical inputs are copied into the host-facing guitar inputs.
  // Corrected host outputs are then copied to physical outputs.
  bool process(const float* const physical_inputs[2],
               float* const physical_outputs[2]) noexcept {
    if (!physical_inputs || !physical_outputs ||
        !physical_inputs[0] || !physical_inputs[1] ||
        !physical_outputs[0] || !physical_outputs[1] ||
        stream_.state()!=VirtualAsioStreamCore::State::running) return false;
    const auto bank=static_cast<std::size_t>(bridge_.completed_blocks()%2);
    const auto bytes=stream_.frames()*sizeof(float);
    for(std::size_t ch=0;ch<2;++ch) {
      auto* destination=stream_.input(bank,ch);
      if (!destination) return false;
      std::memcpy(destination,physical_inputs[ch],bytes);
    }
    if (!bridge_.physical_buffer_ready()) return false;
    for(std::size_t ch=0;ch<2;++ch) {
      auto* source=stream_.output(bank,ch);
      if (!source) return false;
      std::memcpy(physical_outputs[ch],source,bytes);
    }
    return true;
  }
  void stop() noexcept { bridge_.stop(); }
  std::uint64_t completed_blocks() const noexcept {
    return bridge_.completed_blocks();
  }
 private:
  VirtualAsioStreamCore& stream_;
  VirtualAsioClockedBridge bridge_;
};
} // namespace sr850
