#pragma once
// SR850 Universal Audio Bridge: portable, allocation-free-in-callback FIR core.
// This is NOT an ASIO driver. A future ASIO proxy must provide input/output callbacks.
#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace sr850 {
class FirProcessor {
 public:
  // Configure OFF the audio thread. Each channel receives its own history.
  void configure(const std::vector<float>& impulse, std::size_t channels) {
    if (impulse.empty() || channels == 0) throw std::invalid_argument("invalid FIR/channels");
    taps_ = impulse;
    history_.assign(channels * impulse.size(), 0.0f);
    positions_.assign(channels, 0);
    channels_ = channels;
  }

  void reset() noexcept {
    std::fill(history_.begin(), history_.end(), 0.0f);
    std::fill(positions_.begin(), positions_.end(), 0);
  }

  // Interleaved floating-point samples; in-place; no allocations or locks.
  // Bypass passes audio through unchanged; history still advances so toggling
  // does not introduce a stale-buffer burst.
  void process(float* audio, std::size_t frames, float gain, bool enabled) noexcept {
    if (!audio || !channels_ || taps_.empty()) return;
    const std::size_t n = taps_.size();
    for (std::size_t f = 0; f < frames; ++f) {
      for (std::size_t ch = 0; ch < channels_; ++ch) {
        const std::size_t index = f * channels_ + ch;
        const float dry = audio[index];
        std::size_t& pos = positions_[ch];
        float* hist = history_.data() + ch * n;
        hist[pos] = dry;
        float wet = 0.0f;
        std::size_t j = pos;
        for (std::size_t k = 0; k < n; ++k) {
          wet += taps_[k] * hist[j];
          j = j ? j - 1 : n - 1;
        }
        pos = (pos + 1 == n) ? 0 : pos + 1;
        audio[index] = enabled ? wet * gain : dry;
      }
    }
  }
  std::size_t channels() const noexcept { return channels_; }
  std::size_t taps() const noexcept { return taps_.size(); }

 private:
  std::vector<float> taps_;
  std::vector<float> history_;
  std::vector<std::size_t> positions_;
  std::size_t channels_ = 0;
};
} // namespace sr850
