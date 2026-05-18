#ifndef PRJ_NETWORK_H
#define PRJ_NETWORK_H

#include "esp_err.h"

/** Wi-Fi (prj_wifi ou BluFi) + MQTT (prj_mqtt). */
esp_err_t prj_network_init(void);

#endif
