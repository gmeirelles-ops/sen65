#include "acd1200.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

static const char *TAG = "ACD1200";

#define ACD1200_I2C_ADDR 0x2A
#define ACD1200_REQUEST_MS 90
#define ACD1200_POLL_INTERVAL_MS 2000
#define ACD1200_PREHEAT_US (120LL * 1000LL * 1000LL)

static i2c_master_dev_handle_t acd_dev;
static int64_t preheat_start_us;

static portMUX_TYPE snapshot_mux = portMUX_INITIALIZER_UNLOCKED;
static uint16_t latest_ppm;
static bool latest_valid;

static uint8_t acd1200_crc8(const uint8_t *data, int len) {
  uint8_t crc = 0xFF;
  for (int i = 0; i < len; i++) {
    crc ^= data[i];
    for (int b = 0; b < 8; b++) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ 0x31;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

esp_err_t acd1200_init(i2c_master_bus_handle_t i2c_bus) {
  if (i2c_bus == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  i2c_device_config_t dev_cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = ACD1200_I2C_ADDR,
      .scl_speed_hz = 100000,
  };

  esp_err_t err = i2c_master_bus_add_device(i2c_bus, &dev_cfg, &acd_dev);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "add_device: %s", esp_err_to_name(err));
    return err;
  }

  preheat_start_us = esp_timer_get_time();
  latest_valid = false;
  latest_ppm = 0;

  ESP_LOGI(TAG, "ACD1200 I2C 0x%02X (Aosong NDIR / protocolo tipo ACD10)",
           ACD1200_I2C_ADDR);
  return ESP_OK;
}

void acd1200_get_latest(uint16_t *ppm, bool *valid) {
  if (ppm == NULL || valid == NULL) {
    return;
  }
  portENTER_CRITICAL(&snapshot_mux);
  *ppm = latest_ppm;
  *valid = latest_valid;
  portEXIT_CRITICAL(&snapshot_mux);
}

static bool preheat_done(void) {
  return (esp_timer_get_time() - preheat_start_us) >= ACD1200_PREHEAT_US;
}

void acd1200_task(void *pvParameters) {
  (void)pvParameters;
  uint8_t cmd_req[] = {0x03, 0x00};

  while (1) {
    if (!preheat_done()) {
      vTaskDelay(pdMS_TO_TICKS(500));
      continue;
    }

    esp_err_t err = i2c_master_transmit(acd_dev, cmd_req, sizeof(cmd_req), -1);
    if (err != ESP_OK) {
      ESP_LOGW(TAG, "request: %s", esp_err_to_name(err));
      vTaskDelay(pdMS_TO_TICKS(ACD1200_POLL_INTERVAL_MS));
      continue;
    }

    vTaskDelay(pdMS_TO_TICKS(ACD1200_REQUEST_MS));

    uint8_t buf[9];
    memset(buf, 0, sizeof(buf));
    err = i2c_master_receive(acd_dev, buf, sizeof(buf), -1);
    if (err != ESP_OK) {
      ESP_LOGW(TAG, "read: %s", esp_err_to_name(err));
      vTaskDelay(pdMS_TO_TICKS(ACD1200_POLL_INTERVAL_MS));
      continue;
    }

    if (buf[2] != acd1200_crc8(&buf[0], 2) ||
        buf[5] != acd1200_crc8(&buf[3], 2) ||
        buf[8] != acd1200_crc8(&buf[6], 2)) {
      ESP_LOGW(TAG, "CRC CO2");
      vTaskDelay(pdMS_TO_TICKS(ACD1200_POLL_INTERVAL_MS));
      continue;
    }

    uint32_t concentration = (uint32_t)buf[0];
    concentration = (concentration << 8) | buf[1];
    concentration = (concentration << 8) | buf[3];
    concentration = (concentration << 8) | buf[4];
    uint16_t ppm = (uint16_t)concentration;

    portENTER_CRITICAL(&snapshot_mux);
    latest_ppm = ppm;
    latest_valid = true;
    portEXIT_CRITICAL(&snapshot_mux);

    vTaskDelay(pdMS_TO_TICKS(ACD1200_POLL_INTERVAL_MS));
  }
}
