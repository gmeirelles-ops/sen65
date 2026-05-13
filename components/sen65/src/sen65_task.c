#include "sen65_task.h"

#include <string.h>

#include "acd1200.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "sen65.h"

#include "app_config.h"

static const char *TAG = "SEN65_TASK";

static QueueHandle_t sen65_queue;

esp_err_t sen65_task_init(i2c_master_bus_handle_t i2c_bus) {
  sen65_queue = xQueueCreate(SENSOR_QUEUE_SIZE, sizeof(air_raw_data_t));
  if (sen65_queue == NULL) {
    ESP_LOGE(TAG, "fila sensor");
    return ESP_FAIL;
  }

  esp_err_t err = sen65_init(i2c_bus);
  if (err != ESP_OK) {
    return err;
  }

  err = sen65_start();
  if (err != ESP_OK) {
    return err;
  }

  ESP_LOGI(TAG, "Warmup %d ms", SENSOR_WARMUP_TIME_MS);
  vTaskDelay(pdMS_TO_TICKS(SENSOR_WARMUP_TIME_MS));
  ESP_LOGI(TAG, "SEN65 pronto");
  return ESP_OK;
}

QueueHandle_t sen65_get_queue(void) { return sen65_queue; }

esp_err_t sen65_fan_clean(void) {
  (void)TAG;
  return ESP_OK;
}

void sen65_task(void *pvParameters) {
  (void)pvParameters;
  sen65_data_t raw;
  air_raw_data_t data;

  while (1) {
    esp_err_t err = sen65_read(&raw);
    if (err == ESP_OK) {
      data.pm25 =
          (raw.pm2_5 == 0xFFFFu) ? 0u : raw.pm2_5;
      data.voc =
          (raw.voc_index == (int16_t)0x7FFF) ? 0 : raw.voc_index;
      data.nox =
          (raw.nox_index == (int16_t)0x7FFF) ? 0 : raw.nox_index;
      data.rh_ticks = raw.rh_ticks;
      data.temp_ticks = raw.temp_ticks;
      acd1200_get_latest(&data.co2_ppm, &data.co2_valid);

      if (xQueueSend(sen65_queue, &data, pdMS_TO_TICKS(10)) != pdTRUE) {
        ESP_LOGW(TAG, "fila cheia");
      }
    } else {
      ESP_LOGE(TAG, "leitura SEN65: %s", esp_err_to_name(err));
    }
    vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
  }
}
