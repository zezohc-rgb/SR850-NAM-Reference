// Opt-in ASIO COM activation probe for a specific registered driver.
// This is NOT an ASIO audio stream and does not call IASIO methods.
// Usage: asio_activation_probe.exe "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
#include "com_driver_handle_windows.hpp"
#include <iostream>
#include <string>
int wmain(int argc, wchar_t** argv) {
  if (argc != 2) {
    std::wcerr << L"Usage: asio_activation_probe.exe \"{ASIO-driver-CLSID}\"\n";
    return 2;
  }
  const std::wstring clsid(argv[1]);
  CLSID parsed{};
  if (FAILED(CLSIDFromString(clsid.c_str(), &parsed))) {
    std::wcerr << L"Invalid CLSID\n";
    return 2;
  }
  sr850::ComDriverHandle driver;
  const HRESULT hr = driver.open(clsid, IID_IUnknown);
  if (FAILED(hr)) {
    std::wcerr << L"COM activation failed, HRESULT: 0x" << std::hex
               << static_cast<unsigned long>(hr) << L"\n";
    return 1;
  }
  std::wcout << L"COM activation succeeded. Audio was NOT started.\n";
  driver.close();
  return 0;
}
