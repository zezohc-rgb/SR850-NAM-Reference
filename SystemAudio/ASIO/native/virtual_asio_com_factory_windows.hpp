#pragma once
// Safe, unregistered COM class-factory skeleton for future virtual ASIO DLL.
// No IASIO interface is exposed yet: CreateInstance deliberately returns
// E_NOINTERFACE rather than claiming a functional audio driver.
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <unknwn.h>
#include <atomic>
namespace sr850 {
class VirtualAsioClassFactory final : public IClassFactory {
 public:
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out) override {
    if (!out) return E_POINTER;
    *out=nullptr;
    if (IsEqualIID(iid,IID_IUnknown)||IsEqualIID(iid,IID_IClassFactory)) {
      *out=static_cast<IClassFactory*>(this);
      AddRef();
      return S_OK;
    }
    return E_NOINTERFACE;
  }
  ULONG STDMETHODCALLTYPE AddRef() override { return ++references_; }
  ULONG STDMETHODCALLTYPE Release() override {
    const ULONG remaining=--references_;
    if (!remaining) delete this;
    return remaining;
  }
  HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* outer,REFIID,void** out) override {
    if (!out) return E_POINTER;
    *out=nullptr;
    if (outer) return CLASS_E_NOAGGREGATION;
    return E_NOINTERFACE; // IASIO not implemented; fail safely.
  }
  HRESULT STDMETHODCALLTYPE LockServer(BOOL lock) override {
    if (lock) ++server_locks_;
    else {
      auto count=server_locks_.load();
      while (count && !server_locks_.compare_exchange_weak(count,count-1)) {}
    }
    return S_OK;
  }
  static long server_locks() noexcept { return server_locks_.load(); }
 private:
  ~VirtualAsioClassFactory()=default;
  std::atomic<ULONG> references_{1};
  inline static std::atomic<long> server_locks_{0};
};
} // namespace sr850
#endif
