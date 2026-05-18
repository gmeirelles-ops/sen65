#ifndef I2C_BOARD_H
#define I2C_BOARD_H

#include "driver/i2c_master.h"
#include "esp_err.h"

#define BOARD_I2C_PORT I2C_NUM_0
#define BOARD_I2C_SDA_GPIO 26
#define BOARD_I2C_SCL_GPIO 27
#define BOARD_I2C_FREQ_HZ 100000

esp_err_t board_i2c_init(i2c_master_bus_handle_t *out_bus);

#endif
