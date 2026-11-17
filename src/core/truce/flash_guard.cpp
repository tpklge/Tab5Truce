#ifdef TRUCE_SD_ONLY
#include <nvs.h>
#include <nvs_flash.h>
#include <esp_partition.h>
// Arduino calls this before setup(). No NVS partition is initialized or erased.
extern "C" esp_err_t __wrap_nvs_flash_init() { return ESP_OK; }
extern "C" esp_err_t __wrap_nvs_flash_init_partition(const char *) { return ESP_ERR_NOT_SUPPORTED; }
extern "C" esp_err_t __wrap_nvs_flash_erase() { return ESP_ERR_NOT_SUPPORTED; }
extern "C" esp_err_t __wrap_nvs_flash_erase_partition(const char *) { return ESP_ERR_NOT_SUPPORTED; }
extern "C" esp_err_t __wrap_nvs_open(const char *, nvs_open_mode_t, nvs_handle_t *out) {
    if (out) *out = 0;
    return ESP_ERR_NVS_NOT_INITIALIZED;
}
extern "C" esp_err_t __wrap_nvs_open_from_partition(const char *, const char *, nvs_open_mode_t, nvs_handle_t *out) {
    if (out) *out = 0;
    return ESP_ERR_NVS_NOT_INITIALIZED;
}
// No application path (including legacy updaters) may persist data in local flash.
extern "C" esp_err_t __wrap_esp_partition_write(const esp_partition_t *, size_t, const void *, size_t) {
    return ESP_ERR_NOT_SUPPORTED;
}
extern "C" esp_err_t __wrap_esp_partition_erase_range(const esp_partition_t *, size_t, size_t) {
    return ESP_ERR_NOT_SUPPORTED;
}
#endif
