// Read-only Windows ASIO registry discovery. Does not instantiate drivers.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <iostream>
#include <string>
#include <vector>

struct DriverEntry {
  std::wstring name;
  std::wstring clsid;
  REGSAM registry_view;
};

std::wstring read_string(HKEY key, const wchar_t* value_name) {
  DWORD type = 0;
  DWORD bytes = 0;
  if (RegQueryValueExW(key, value_name, nullptr, &type, nullptr, &bytes) != ERROR_SUCCESS ||
      (type != REG_SZ && type != REG_EXPAND_SZ) || bytes < sizeof(wchar_t) ||
      bytes > 32768) return {};
  std::vector<wchar_t> value(bytes / sizeof(wchar_t) + 1, L'\0');
  if (RegQueryValueExW(key, value_name, nullptr, &type,
                       reinterpret_cast<BYTE*>(value.data()), &bytes) != ERROR_SUCCESS)
    return {};
  value.back() = L'\0';
  return value.data();
}

std::vector<DriverEntry> enumerate_asio(REGSAM view) {
  std::vector<DriverEntry> entries;
  HKEY root = nullptr;
  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\ASIO", 0,
                    KEY_READ | view, &root) != ERROR_SUCCESS) return entries;
  for (DWORD index = 0;; ++index) {
    wchar_t name[512] = {};
    DWORD length = 512;
    const LONG result = RegEnumKeyExW(root, index, name, &length, nullptr,
                                      nullptr, nullptr, nullptr);
    if (result == ERROR_NO_MORE_ITEMS) break;
    if (result != ERROR_SUCCESS) continue;
    HKEY child = nullptr;
    if (RegOpenKeyExW(root, name, 0, KEY_READ | view, &child) != ERROR_SUCCESS)
      continue;
    std::wstring clsid = read_string(child, L"CLSID");
    RegCloseKey(child);
    CLSID parsed{};
    if (!clsid.empty() && SUCCEEDED(CLSIDFromString(clsid.c_str(), &parsed)))
      entries.push_back({name, clsid, view});
  }
  RegCloseKey(root);
  return entries;
}

int wmain() {
  std::wcout << L"SR850 ASIO registry discovery (read-only)\n";
  for (const REGSAM view : {KEY_WOW64_64KEY, KEY_WOW64_32KEY}) {
    std::wcout << (view == KEY_WOW64_64KEY ? L"64-bit:\n" : L"32-bit:\n");
    const auto entries = enumerate_asio(view);
    if (entries.empty()) std::wcout << L"  No valid ASIO registrations found\n";
    for (const auto& entry : entries)
      std::wcout << L"  " << entry.name << L" | " << entry.clsid << L"\n";
  }
  return 0;
}
