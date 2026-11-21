#pragma once
#include "stream_negotiation.hpp"

namespace sr850 {
// SDK-independent contract for a real ASIO backend. The concrete Windows
// implementation must invoke IASIO and own driver callbacks and buffers.
class AsioBackend {
 public:
  virtual ~AsioBackend() = default;
  virtual bool open() = 0;
  virtual bool query_stream(StreamFormat& format) = 0;
  virtual bool prepare(const StreamFormat& format) = 0;
  virtual bool start() = 0;
  virtual bool stop() = 0;
  virtual void close() = 0;
};

class BackendController {
 public:
  explicit BackendController(AsioBackend& backend) noexcept : backend_(backend) {}
  BackendController(const BackendController&) = delete;
  BackendController& operator=(const BackendController&) = delete;
  ~BackendController() { close(); }

  bool open_and_prepare(const StreamFormat& app, std::size_t max_frames) {
    if (state_ != State::closed) return false;
    if (!backend_.open()) { backend_.close(); return false; }
    StreamFormat device{};
    if (!backend_.query_stream(device) ||
        negotiate_direct(app, device, max_frames) != NegotiationResult::ok ||
        !backend_.prepare(app)) {
      backend_.close();
      return false;
    }
    state_ = State::prepared;
    return true;
  }
  bool start() {
    if (state_ != State::prepared || !backend_.start()) return false;
    state_ = State::running;
    return true;
  }
  bool stop() {
    if (state_ != State::running || !backend_.stop()) return false;
    state_ = State::prepared;
    return true;
  }
  void close() noexcept {
    if (state_ == State::running) backend_.stop();
    backend_.close();
    state_ = State::closed;
  }
  bool running() const noexcept { return state_ == State::running; }
 private:
  enum class State { closed, prepared, running };
  AsioBackend& backend_;
  State state_ = State::closed;
};
} // namespace sr850
