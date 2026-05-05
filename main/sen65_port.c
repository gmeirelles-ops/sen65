#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"

/* Ajuste aos pinos da tua placa; SDA/SCL não podem estar trocados. */
#define SDA_PIN 26
#define SCL_PIN 27
#define FREQ    50000
#define SEN65_ADDR 0x6B

/*
 * O driver interno do ESP32 (~45kΩ) costuma ser fraco para I2C com fios longos
 * ou com o barramento do SEN65. Use resistências de pull-up externas 4,7kΩ
 * (ou 10kΩ) entre SDA→3V3 e SCL→3V3 se aparecer "hardware timeout" ou NACK.
 */

static const char *TAG = "SEN65_PORT";
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t dev_handle;

esp_err_t sen65_port_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = SCL_PIN,
        .sda_io_num = SDA_PIN,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t ret = i2c_new_master_bus(&bus_config, &bus_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = i2c_master_probe(bus_handle, SEN65_ADDR, 200);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG,
                 "SEN65 não responde no endereço 0x%02X (SDA=%d SCL=%d). " ,SEN65_ADDR, SDA_PIN, SCL_PIN);
        i2c_del_master_bus(bus_handle);
        bus_handle = NULL;
        return ret;
    }

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SEN65_ADDR,
        .scl_speed_hz = FREQ,
        .scl_wait_us = 80000,
    };

    ret = i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle);
    if (ret != ESP_OK) {
        i2c_del_master_bus(bus_handle);
        bus_handle = NULL;
    }
    return ret;
}

esp_err_t sen65_port_write(uint16_t cmd)
{
    uint8_t data[2] = { (uint8_t)(cmd >> 8), (uint8_t)(cmd & 0xFF) };

    
    return i2c_master_transmit(dev_handle, data, 2, 500);
}

esp_err_t sen65_port_read(uint8_t *buf, size_t len)
{
    return i2c_master_receive(dev_handle, buf, len, 500);
}