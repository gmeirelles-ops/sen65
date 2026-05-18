#ifndef PRJ_WIFI_H
#define PRJ_WIFI_H

#include <stdbool.h>

#include "esp_err.h"

/** Aplica credenciais NVS e aguarda IP (stack Wi-Fi ja iniciado). */
esp_err_t prj_wifi_init(void);

/** true enquanto STA tenta obter IP (evita scan BluFi em paralelo). */
bool prj_wifi_sta_busy(void);

#endif
