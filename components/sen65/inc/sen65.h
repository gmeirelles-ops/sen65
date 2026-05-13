#ifndef SEN65_H
#define SEN65_H

#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

typedef struct {
  uint16_t pm1_0;
  uint16_t pm2_5;
  uint16_t pm4_0;
  uint16_t pm10;
  int16_t rh_ticks;
  int16_t temp_ticks;
  int16_t voc_index;
  int16_t nox_index;
} sen65_data_t;

esp_err_t sen65_init(i2c_master_bus_handle_t i2c_bus);

esp_err_t sen65_start(void);

esp_err_t sen65_read(sen65_data_t *data);

#endif
