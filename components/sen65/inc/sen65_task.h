#ifndef SEN65_TASK_H
#define SEN65_TASK_H

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "types.h"

esp_err_t sen65_task_init(i2c_master_bus_handle_t i2c_bus);

void sen65_task(void *pvParameters);

QueueHandle_t sen65_get_queue(void);

esp_err_t sen65_fan_clean(void);

#endif
