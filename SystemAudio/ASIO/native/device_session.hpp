#pragma once
#include "stream_negotiation.hpp"
#include <cstddef>

namespace sr850 {
// State machine for the eventual physical ASIO device adapter.
// Does not load drivers or call an ASIO SDK; those require a backend.
enum class SessionState { closed, configured, running };
enum class SessionError { ok, invalid_state, unsupported_stream };
class DeviceSession {
 public:
  SessionError configure(const StreamFormat& app, const StreamFormat& device,
                         std::size_t max_frames) noexcept {
    if (state_ != SessionState::closed) return SessionError::invalid_state;
    if (negotiate_direct(app, device, max_frames) != NegotiationResult::ok)
      return SessionError::unsupported_stream;
    format_ = app;
    state_ = SessionState::configured;
    return SessionError::ok;
  }
  SessionError start() noexcept {
    if (state_ != SessionState::configured) return SessionError::invalid_state;
    state_ = SessionState::running;
    return SessionError::ok;
  }
  SessionError stop() noexcept {
    if (state_ != SessionState::running) return SessionError::invalid_state;
    state_ = SessionState::configured;
    return SessionError::ok;
  }
  void close() noexcept {
    state_ = SessionState::closed;
    format_ = {};
  }
  SessionState state() const noexcept { return state_; }
  const StreamFormat& format() const noexcept { return format_; }
 private:
  SessionState state_ = SessionState::closed;
  StreamFormat format_{};
};
} // namespace sr850
