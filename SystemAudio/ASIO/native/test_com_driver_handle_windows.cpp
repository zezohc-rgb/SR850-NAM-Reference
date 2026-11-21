#include "com_driver_handle_windows.hpp"
#include <cassert>
#include <iostream>
int main() {
  sr850::ComDriverHandle handle;
  assert(!handle.active());
  const HRESULT result = handle.open(L"not-a-clsid", IID_IUnknown);
  assert(FAILED(result));
  assert(!handle.active());
  handle.close();
  std::cout << "COM activation guard smoke test passed\n";
}
