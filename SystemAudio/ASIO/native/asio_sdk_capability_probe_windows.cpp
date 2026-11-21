#ifdef _WIN32
// Opt-in ASIO SDK probe. Requires the official Steinberg ASIO SDK headers.
// Never creates buffers, starts the driver, or streams audio.
// Build (MSVC Developer Prompt):
// cl /nologo /std:c++17 /EHsc /W4 /I"path\\to\\ASIOSDK\\common" asio_sdk_capability_probe_windows.cpp ole32.lib
#include <windows.h>
#include <objbase.h>
#include <cstdio>
#include <cwchar>
#include "iasiodrv.h" // Defines the IASIO COM interface (asio.h alone does not).

int wmain(int argc, wchar_t** argv) {
  if (argc != 3 || std::wcscmp(argv[1], L"--i-accept-driver-risk") != 0) {
    std::fwprintf(stderr, L"Usage: probe --i-accept-driver-risk {ASIO-DRIVER-CLSID}\\n");
    return 2;
  }
  CLSID clsid{};
  if (FAILED(CLSIDFromString(argv[2], &clsid))) return 3;
  const HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  if (FAILED(init)) return 4;
  // The Steinberg ASIO COM convention uses the driver's CLSID as the
  // requested interface identifier; this is NOT IID_IUnknown.
  IASIO* driver = nullptr;
  const HRESULT created = CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER,
                                            clsid, reinterpret_cast<void**>(&driver));
  if (FAILED(created) || !driver) {
    std::fwprintf(stderr, L"ASIO COM interface activation failed: 0x%08lX\\n",
                  static_cast<unsigned long>(created));
    CoUninitialize();
    return 5;
  }
  // init() can load device-specific code and may destabilize legacy drivers.
  // Explicit flag is required, and this probe must never run in CI.
  // IASIO::init returns ASIOBool; no audio stream is started.
  const ASIOBool ready = driver->init(GetDesktopWindow());
  if (!ready) {
    std::fprintf(stderr, "ASIO init failed. No buffers were created.\\n");
    driver->Release();
    CoUninitialize();
    return 6;
  }
  long inputs=0, outputs=0, minimum=0, maximum=0, preferred=0, granularity=0;
  ASIOSampleRate rate=0;
  const ASIOError channels=driver->getChannels(&inputs,&outputs);
  const ASIOError buffers=driver->getBufferSize(&minimum,&maximum,&preferred,&granularity);
  const ASIOError sample_rate=driver->getSampleRate(&rate);
  bool channel_info_ok = true;
  if (channels == ASE_OK && inputs >= 0 && outputs >= 0 && inputs <= 64 && outputs <= 64) {
    for (long direction = 0; direction < 2; ++direction) {
      const long count = direction == 0 ? inputs : outputs;
      for (long index = 0; index < count; ++index) {
        ASIOChannelInfo info{};
        info.channel = index;
        info.isInput = direction == 0 ? ASIOTrue : ASIOFalse;
        const ASIOError result = driver->getChannelInfo(&info);
        if (result != ASE_OK) { channel_info_ok = false; continue; }
        std::printf("%s channel %ld: sample-type=%d active=%d name=%s\\n",
                    direction == 0 ? "Input" : "Output", index,
                    static_cast<int>(info.type), static_cast<int>(info.isActive), info.name);
      }
    }
  } else { channel_info_ok = false; }
  if (channels == ASE_OK && buffers == ASE_OK && sample_rate == ASE_OK) {
    std::printf("Inputs: %ld; outputs: %ld\\n",inputs,outputs);
    std::printf("Buffer: min=%ld max=%ld preferred=%ld granularity=%ld\\n",
                minimum,maximum,preferred,granularity);
    std::printf("Sample rate: %.0f Hz\\n",rate);
  } else {
    std::fprintf(stderr, "One or more ASIO capability queries failed.\\n");
  }
  driver->Release();
  CoUninitialize();
  return (channels == ASE_OK && buffers == ASE_OK && sample_rate == ASE_OK && channel_info_ok) ? 0 : 7;
}
#else
#error This probe is Windows-only.
#endif
