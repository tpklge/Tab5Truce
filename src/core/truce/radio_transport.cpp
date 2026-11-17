#ifdef TRUCE_LVGL
#include <Arduino.h>
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#include <esp32-hal-hosted.h>
#include <truce_radio_policy.h>
namespace truce {
static bool bleTransportReady=false;
static const char *bleFailure="Memória interna insuficiente para iniciar o Bluetooth.";
bool prepareHostedBle() {
    bleFailure="Memória interna insuficiente para iniciar o Bluetooth.";
    if(bleTransportReady)return true;
    // Register SDIO pins and bring up the C6 before NimBLE's SDK HCI init,
    // which otherwise aborts on an unconfigured transport. Leave boot radio-free.
    if(!hostedInitBLE()) {
        bleFailure="Bluetooth indisponível: não foi possível iniciar o enlace com o C6.";
        return false;
    }
    bleTransportReady=true;
    return true;
}
const char *hostedBleFailure() { return bleFailure; }
}
#endif

#endif
