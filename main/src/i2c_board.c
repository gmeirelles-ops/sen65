#include "i2c_board.h"

#include "esp_log.h"

static const char *TAG = "I2C_BOARD";

esp_err_t board_i2c_init(i2c_master_bus_handle_t *out_bus) {
  if (out_bus == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  i2c_master_bus_config_t bus_config = {
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .i2c_port = BOARD_I2C_PORT,
      .sda_io_num = BOARD_I2C_SDA_GPIO,
      .scl_io_num = BOARD_I2C_SCL_GPIO,
      .glitch_ignore_cnt = 7,
      .flags.enable_internal_pullup = true,
  };

  esp_err_t err = i2c_new_master_bus(&bus_config, out_bus);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "i2c_new_master_bus: %s", esp_err_to_name(err));
    return err;
  }

  ESP_LOGI(TAG, "I2C SDA=%d SCL=%d", BOARD_I2C_SDA_GPIO, BOARD_I2C_SCL_GPIO);
  return ESP_OK;
}
