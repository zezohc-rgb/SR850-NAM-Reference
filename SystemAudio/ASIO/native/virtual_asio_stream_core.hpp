#pragma once
// Host-facing virtual ASIO stream state. Pure C++: no driver registration,
// COM ABI or hardware access. Enables deterministic offline testing.
#include <cstddef>
#include <cstdint>
#include <vector>

namespace sr850 {
class VirtualAsioStreamCore {
 public:
  enum class State { closed, prepared, running };
  bool prepare(std::size_t frames, std::size_t input_channels,
               std::size_t output_channels) {
    if (state_!=State::closed || frames==0 || frames>8192 ||
        input_channels==0 || input_channels>64 ||
        output_channels==0 || output_channels>64) return false;
    frames_=frames;
    inputs_.assign(input_channels*frames*2,0.0f);
    outputs_.assign(output_channels*frames*2,0.0f);
    input_channels_=input_channels;
    output_channels_=output_channels;
    state_=State::prepared;
    return true;
  }
  bool start() noexcept {
    if (state_!=State::prepared) return false;
    state_=State::running;
    return true;
  }
  void stop() noexcept {
    if (state_==State::running) state_=State::prepared;
  }
  void close() noexcept {
    stop();
    inputs_.clear(); outputs_.clear();
    frames_=input_channels_=output_channels_=0;
    state_=State::closed;
  }
  float* input(std::size_t bank,std::size_t channel) noexcept {
    if (state_==State::closed || bank>1 || channel>=input_channels_) return nullptr;
    return inputs_.data()+(bank*input_channels_+channel)*frames_;
  }
  float* output(std::size_t bank,std::size_t channel) noexcept {
    if (state_==State::closed || bank>1 || channel>=output_channels_) return nullptr;
    return outputs_.data()+(bank*output_channels_+channel)*frames_;
  }
  std::size_t frames() const noexcept { return frames_; }
  State state() const noexcept { return state_; }
  ~VirtualAsioStreamCore() { close(); }
 private:
  std::vector<float> inputs_,outputs_;
  std::size_t frames_=0,input_channels_=0,output_channels_=0;
  State state_=State::closed;
};
} // namespace sr850
