#ifndef MQTT_AIR_H
#define MQTT_AIR_H

#include "esp_err.h"
#include "types.h"

esp_err_t mqtt_air_init(void);

void mqtt_air_publish(const air_processed_data_t *processed);

#endif
