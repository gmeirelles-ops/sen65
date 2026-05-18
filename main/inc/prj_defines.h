#ifndef PRJ_DEFINES_H
#define PRJ_DEFINES_H

#include "app_config.h"

/** NVS — credenciais Wi-Fi / MQTT (prj_data_t). */
#define PRJ_NVS_NAMESPACE "nvs"
#define PRJ_NVS_KEY_CFG   "prj_cfg"

/** Timeout do modo pareamento BluFi (ms). */
#define PRJ_PAIR_TIMEOUT PRJ_WIFI_PAIRING_WINDOW_MS

/** Espera por IP apos esp_wifi_start (ms). */
#define PRJ_WIFI_CONN_TIMEOUT_MS (30 * 1000)

#endif
