#pragma once
// Host-facing ASIO driver capability model. No SDK dependency; this layer
// describes a virtual driver but does not register a COM class.
#include <cstddef>
#include <cstdint>

namespace sr850 {
struct VirtualAsioCapabilities {
  static constexpr long input_channels=2;
  static constexpr long output_channels=2;
  static constexpr long minimum_frames=64;
  static constexpr long maximum_frames=1024;
  static constexpr long preferred_frames=256;
  static constexpr long buffer_granularity=-1; // ASIO: powers of two
  static constexpr long sample_type_float32_lsb=19; // ASIOSTFloat32LSB
  static bool supported_rate(double hz) noexcept {
    return hz==44100.0 || hz==48000.0;
  }
  static bool supported_frames(long frames) noexcept {
    return frames>=minimum_frames && frames<=maximum_frames &&
           (frames & (frames-1))==0;
  }
  static bool supported_channel(long index,bool input) noexcept {
    return index>=0 && index<(input ? input_channels:output_channels);
  }
};
} // namespace sr850
