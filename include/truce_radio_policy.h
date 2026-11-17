#pragma once
#include <stddef.h>
namespace truce {
#if defined(TRUCE_LVGL) && defined(CONFIG_IDF_TARGET_ESP32P4)
bool prepareHostedBle();
const char *hostedBleFailure();
#endif
// P4 hosts NimBLE; the C6 owns the controller. Still reserve internal headroom
// for host stacks and transport buffers instead of accepting total PSRAM as RAM.
inline bool hostedBleHeadroom(size_t freeInternal, size_t largestInternal) {
    return freeInternal >= 64 * 1024 && largestInternal >= 16 * 1024;
}
}
