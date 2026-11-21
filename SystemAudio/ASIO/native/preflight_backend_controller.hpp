#pragma once
// Adapter for ASIO-capability-aware backends. No driver is loaded here.
#include "asio_backend.hpp"
#include "asio_preflight.hpp"
#include "format_preflight.hpp"

namespace sr850 {
class PreflightBackendController {
 public:
  PreflightBackendController(AsioBackend& backend,
                             AsioCapabilitySource& source,
                             ConversionSupport conversion = {}) noexcept
      : backend_(backend), source_(source), conversion_(conversion) {}
  PreflightBackendController(const PreflightBackendController&) = delete;
  PreflightBackendController& operator=(const PreflightBackendController&) = delete;
  ~PreflightBackendController() { close(); }

  bool prepare(const StreamFormat& requested, std::size_t max_frames) {
    if (state_ != State::closed) return false;
    if (!backend_.open()) { backend_.close(); return false; }
    AsioCapabilities caps{};
    StreamFormat reported{};
    if (preflight_asio_stream(source_, requested, max_frames, caps) !=
            AsioPreflightError::ok ||
        !backend_.query_stream(reported) ||
        negotiate_with_conversion(requested, reported, max_frames, conversion_) != NegotiationResult::ok ||
        !backend_.prepare(requested)) {
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
    if (state_ == State::closed) return;
    if (state_ == State::running) backend_.stop();
    backend_.close();
    state_ = State::closed;
  }
  bool running() const noexcept { return state_ == State::running; }
 private:
  enum class State { closed, prepared, running };
  AsioBackend& backend_;
  AsioCapabilitySource& source_;
  ConversionSupport conversion_;
  State state_ = State::closed;
};
} // namespace sr850
