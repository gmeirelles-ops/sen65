#include "sen65.h"

#include <string.h>

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

//====================================================
// CONFIG
//====================================================

#define SEN65_I2C_FREQ_HZ 100000

#define SEN65_ADDR 0x6B

//====================================================
// COMMANDS
//====================================================

#define SEN65_CMD_START 0x0021
#define SEN65_CMD_READ 0x0446

//====================================================
// TAG
//====================================================

static const char *TAG = "SEN65";

//====================================================
// I2C HANDLE
//====================================================

static i2c_master_dev_handle_t sen65_dev;

//====================================================
// CRC
//====================================================

static uint8_t sen65_crc(const uint8_t *data, int len) {
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

static int16_t sen65_wire_voc_nox_to_index(int16_t wire) {
  if (wire == (int16_t)0x7FFF) {
    return (int16_t)0x7FFF;
  }
  return (int16_t)(((int32_t)wire) / 10);
}

static uint16_t sen65_wire_pm_u16(uint8_t msb, uint8_t lsb) {
  return (uint16_t)(((uint16_t)msb << 8) | (uint16_t)lsb);
}

//====================================================
// WRITE COMMAND
//====================================================

static esp_err_t sen65_write_cmd(uint16_t cmd) {
  uint8_t tx[2];

  tx[0] = (cmd >> 8) & 0xFF;
  tx[1] = cmd & 0xFF;

  return i2c_master_transmit(sen65_dev, tx, sizeof(tx), -1);
}

//====================================================
// INIT
//====================================================

esp_err_t sen65_init(i2c_master_bus_handle_t i2c_bus) {
  if (i2c_bus == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  i2c_device_config_t dev_cfg = {

      .dev_addr_length = I2C_ADDR_BIT_LEN_7,

      .device_address = SEN65_ADDR,

      .scl_speed_hz = SEN65_I2C_FREQ_HZ};

  esp_err_t err = i2c_master_bus_add_device(i2c_bus, &dev_cfg, &sen65_dev);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Falha ao adicionar SEN65 ao barramento I2C");
    return err;
  }

  ESP_LOGI(TAG, "SEN65 inicializado");

  return ESP_OK;
}

//====================================================
// START MEASUREMENTS
//====================================================

esp_err_t sen65_start(void) {
  esp_err_t err = sen65_write_cmd(SEN65_CMD_START);

  if (err == ESP_OK) {

    ESP_LOGI(TAG, "Medicao iniciada");
  }

  return err;
}

//====================================================
// READ
//====================================================

esp_err_t sen65_read(sen65_data_t *out) {
  if (out == NULL) {

    return ESP_ERR_INVALID_ARG;
  }

  //------------------------------------------------
  // SEND READ CMD
  //------------------------------------------------

  esp_err_t err = sen65_write_cmd(SEN65_CMD_READ);

  if (err != ESP_OK) {

    return err;
  }

  //------------------------------------------------
  // WAIT
  //------------------------------------------------

  vTaskDelay(pdMS_TO_TICKS(20));

  //------------------------------------------------
  // RX BUFFER
  //------------------------------------------------

  uint8_t rx[24];

  memset(rx, 0, sizeof(rx));

  //------------------------------------------------
  // READ
  //------------------------------------------------

  err = i2c_master_receive(sen65_dev, rx, sizeof(rx), -1);

  if (err != ESP_OK) {

    return err;
  }

  //------------------------------------------------
  // CRC CHECK
  //------------------------------------------------

  for (int i = 0; i < 24; i += 3) {

    uint8_t crc = sen65_crc(&rx[i], 2);

    if (crc != rx[i + 2]) {

      ESP_LOGE(TAG, "CRC error");

      return ESP_ERR_INVALID_CRC;
    }
  }

  //------------------------------------------------
  // PARSE
  //------------------------------------------------

  out->pm1_0 = sen65_wire_pm_u16(rx[0], rx[1]);
  out->pm2_5 = sen65_wire_pm_u16(rx[3], rx[4]);
  out->pm4_0 = sen65_wire_pm_u16(rx[6], rx[7]);
  out->pm10 = sen65_wire_pm_u16(rx[9], rx[10]);

  out->rh_ticks = (int16_t)((rx[12] << 8) | rx[13]);
  out->temp_ticks = (int16_t)((rx[15] << 8) | rx[16]);

  out->voc_index =
      sen65_wire_voc_nox_to_index((int16_t)((rx[18] << 8) | rx[19]));
  out->nox_index =
      sen65_wire_voc_nox_to_index((int16_t)((rx[21] << 8) | rx[22]));

  return ESP_OK;
}