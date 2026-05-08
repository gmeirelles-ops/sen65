#include "sen65.h"

#include <string.h>

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

//====================================================
// CONFIG
//====================================================

#define SEN65_I2C_PORT         I2C_NUM_0

#define SEN65_SDA_GPIO         26
#define SEN65_SCL_GPIO         27

#define SEN65_I2C_FREQ_HZ      100000

#define SEN65_ADDR             0x6B

//====================================================
// COMMANDS
//====================================================

#define SEN65_CMD_START        0x0021
#define SEN65_CMD_READ         0x0446

//====================================================
// TAG
//====================================================

static const char *TAG = "SEN65";

//====================================================
// I2C HANDLE
//====================================================

static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t sen65_dev;

//====================================================
// CRC
//====================================================

static uint8_t sen65_crc(
    const uint8_t *data,
    int len)
{
    uint8_t crc = 0xFF;

    for (int i = 0; i < len; i++) {

        crc ^= data[i];

        for (int b = 0; b < 8; b++) {

            if (crc & 0x80) {

                crc =
                    (crc << 1) ^ 0x31;

            } else {

                crc <<= 1;
            }
        }
    }

    return crc;
}

//====================================================
// WRITE COMMAND
//====================================================

static esp_err_t sen65_write_cmd(
    uint16_t cmd)
{
    uint8_t tx[2];

    tx[0] = (cmd >> 8) & 0xFF;
    tx[1] = cmd & 0xFF;

    return i2c_master_transmit(
        sen65_dev,
        tx,
        sizeof(tx),
        -1);
}

//====================================================
// INIT
//====================================================

esp_err_t sen65_init(void)
{
    //------------------------------------------------
    // BUS CONFIG
    //------------------------------------------------

    i2c_master_bus_config_t bus_config = {

        .clk_source =
            I2C_CLK_SRC_DEFAULT,

        .i2c_port =
            SEN65_I2C_PORT,

        .sda_io_num =
            SEN65_SDA_GPIO,

        .scl_io_num =
            SEN65_SCL_GPIO,

        .glitch_ignore_cnt =
            7,

        .flags.enable_internal_pullup =
            true
    };

    //------------------------------------------------
    // NEW BUS
    //------------------------------------------------

    ESP_ERROR_CHECK(
        i2c_new_master_bus(
            &bus_config,
            &bus_handle));

    //------------------------------------------------
    // DEVICE CONFIG
    //------------------------------------------------

    i2c_device_config_t dev_cfg = {

        .dev_addr_length =
            I2C_ADDR_BIT_LEN_7,

        .device_address =
            SEN65_ADDR,

        .scl_speed_hz =
            SEN65_I2C_FREQ_HZ
    };

    //------------------------------------------------
    // ADD DEVICE
    //------------------------------------------------

    ESP_ERROR_CHECK(
        i2c_master_bus_add_device(
            bus_handle,
            &dev_cfg,
            &sen65_dev));

    ESP_LOGI(TAG,
             "SEN65 inicializado");

    return ESP_OK;
}

//====================================================
// START MEASUREMENTS
//====================================================

esp_err_t sen65_start(void)
{
    esp_err_t err =
        sen65_write_cmd(
            SEN65_CMD_START);

    if (err == ESP_OK) {

        ESP_LOGI(TAG,
                 "Medicao iniciada");
    }

    return err;
}

//====================================================
// READ
//====================================================

esp_err_t sen65_read(
    sen65_data_t *out)
{
    if (out == NULL) {

        return ESP_ERR_INVALID_ARG;
    }

    //------------------------------------------------
    // SEND READ CMD
    //------------------------------------------------

    esp_err_t err =
        sen65_write_cmd(
            SEN65_CMD_READ);

    if (err != ESP_OK) {

        return err;
    }

    //------------------------------------------------
    // WAIT
    //------------------------------------------------

    vTaskDelay(
        pdMS_TO_TICKS(20));

    //------------------------------------------------
    // RX BUFFER
    //------------------------------------------------

    uint8_t rx[24];

    memset(rx, 0, sizeof(rx));

    //------------------------------------------------
    // READ
    //------------------------------------------------

    err =
        i2c_master_receive(
            sen65_dev,
            rx,
            sizeof(rx),
            -1);

    if (err != ESP_OK) {

        return err;
    }

    //------------------------------------------------
    // CRC CHECK
    //------------------------------------------------

    for (int i = 0; i < 24; i += 3) {

        uint8_t crc =
            sen65_crc(&rx[i], 2);

        if (crc != rx[i + 2]) {

            ESP_LOGE(TAG,
                     "CRC error");

            return ESP_ERR_INVALID_CRC;
        }
    }

    //------------------------------------------------
    // PARSE
    //------------------------------------------------

    out->pm1_0 =
        (rx[0] << 8) | rx[1];

    out->pm2_5 =
        (rx[3] << 8) | rx[4];

    out->pm4_0 =
        (rx[6] << 8) | rx[7];

    out->pm10 =
        (rx[9] << 8) | rx[10];

    out->humidity =
        (rx[12] << 8) | rx[13];

    out->temperature =
        (rx[15] << 8) | rx[16];

    out->voc_index =
        (rx[18] << 8) | rx[19];

    out->nox_index =
        (rx[21] << 8) | rx[22];

    return ESP_OK;
}