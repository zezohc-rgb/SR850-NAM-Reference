// Compile-only verification of official ASIO callback signatures.
// Does not initialize, activate, or open any physical ASIO driver.
#ifdef _WIN32
#include "asio_sdk_callback_dispatch_windows.hpp"
#include <type_traits>
static_assert(std::is_same_v<decltype(sr850::AsioSdkCallbackDispatch::callbacks()),
                             ASIOCallbacks>, "ASIO callback table type mismatch");
int main() {
  const ASIOCallbacks cb=sr850::AsioSdkCallbackDispatch::callbacks();
  return cb.bufferSwitch && cb.bufferSwitchTimeInfo &&
         cb.asioMessage && cb.sampleRateDidChange ? 0 : 1;
}
#else
#error Windows-only ASIO SDK compile check.
#endif
