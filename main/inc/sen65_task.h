#ifndef SEN65_TASK_H
#define SEN65_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_err.h"

#include "types.h"

//====================================================
// INIT
//====================================================

esp_err_t sen65_task_init(void);

//====================================================
// TASK
//====================================================

void sen65_task(void *pvParameters);

//====================================================
// QUEUE
//====================================================

QueueHandle_t sen65_get_queue(void);

//====================================================
// CLEAN
//====================================================

esp_err_t sen65_fan_clean(void);

#endif