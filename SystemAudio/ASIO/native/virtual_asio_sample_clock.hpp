#pragma once
// Clock accounting for an externally clocked ASIO bridge.
// This class never creates a software timer: the physical audio callback
// must call advance() once for each completed block.
#include <cstdint>
#include <limits>
namespace sr850 {
class VirtualAsioSampleClock {
 public:
  bool configure(std::uint32_t frames,std::uint32_t sample_rate) noexcept {
    if (!frames || (sample_rate!=44100 && sample_rate!=48000)) return false;
    frames_=frames;
    sample_rate_=sample_rate;
    position_=0;
    blocks_=0;
    return true;
  }
  bool advance() noexcept {
    if (!frames_ || position_>std::numeric_limits<std::uint64_t>::max()-frames_)
      return false;
    position_+=frames_;
    ++blocks_;
    return true;
  }
  void reset() noexcept { position_=0;blocks_=0; }
  std::uint64_t position() const noexcept { return position_; }
  std::uint64_t blocks() const noexcept { return blocks_; }
  std::uint32_t sample_rate() const noexcept { return sample_rate_; }
  std::uint32_t frames() const noexcept { return frames_; }
  double seconds() const noexcept {
    return sample_rate_ ? static_cast<double>(position_)/sample_rate_ : 0.0;
  }
 private:
  std::uint64_t position_=0,blocks_=0;
  std::uint32_t frames_=0,sample_rate_=0;
};
} // namespace sr850
