#pragma once
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <string>
#include <utility>

namespace sr850 {
// Windows COM activation primitive for an ASIO registry CLSID.
// Activation alone does NOT establish an ASIO session. The ASIO SDK
// interface and driver lifecycle must be integrated separately.
class ComDriverHandle {
 public:
  ComDriverHandle() = default;
  ComDriverHandle(const ComDriverHandle&) = delete;
  ComDriverHandle& operator=(const ComDriverHandle&) = delete;
  ~ComDriverHandle() { close(); }
  HRESULT open(const std::wstring& clsid_string, REFIID interface_id) noexcept {
    close();
    CLSID clsid{};
    const HRESULT parsed = CLSIDFromString(clsid_string.c_str(), &clsid);
    if (FAILED(parsed)) return parsed;
    const HRESULT initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(initialized)) return initialized;
    com_initialized_ = true;
    const HRESULT created = CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER,
                                              interface_id, &instance_);
    if (FAILED(created)) close();
    return created;
  }
  void close() noexcept {
    if (instance_) { static_cast<IUnknown*>(instance_)->Release(); instance_ = nullptr; }
    if (com_initialized_) { CoUninitialize(); com_initialized_ = false; }
  }
  bool active() const noexcept { return instance_ != nullptr; }
  // COM initialization, use and close must occur on the same thread.
  // Fast Track's registered COM server declares ThreadingModel=Apartment.
  // This handle must not be moved to or destroyed from another thread.
 private:
  void* instance_ = nullptr;
  bool com_initialized_ = false;
};
} // namespace sr850
#endif
