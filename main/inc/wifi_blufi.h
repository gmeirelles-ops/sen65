#ifndef WIFI_BLUFI_H
#define WIFI_BLUFI_H

#include "esp_err.h"

/** So Wi-Fi (sem BLE) — usar antes de prj_wifi_init se ja houver SSID na NVS. */
esp_err_t wifi_blufi_ensure_wifi_stack(void);
/** BLE/BluFi (requer ensure_wifi_stack). */
esp_err_t wifi_blufi_start_ble(void);
/** Wi-Fi + BLE (pareamento / sem credenciais). */
esp_err_t wifi_blufi_start(void);
void wifi_blufi_advertise(void);
esp_err_t wifi_blufi_stop(void);
const char *wifi_blufi_gap_name(void);

#endif
