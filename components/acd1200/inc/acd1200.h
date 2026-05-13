#ifndef ACD1200_H
#define ACD1200_H

#include <stdbool.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

#define ACD1200_TASK_STACK 3072
#define ACD1200_TASK_PRIORITY 5

esp_err_t acd1200_init(i2c_master_bus_handle_t i2c_bus);

void acd1200_task(void *pvParameters);

void acd1200_get_latest(uint16_t *ppm, bool *valid);

#endif
