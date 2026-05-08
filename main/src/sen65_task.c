#include "sen65_task.h"

#include <string.h>

#include "freertos/task.h"

#include "esp_log.h"
#include "sen65.h"

#include "app_config.h"

//====================================================
// TAG
//====================================================

static const char *TAG = "SEN65_TASK";

//====================================================
// QUEUE
//====================================================

static QueueHandle_t sen65_queue = NULL;

//====================================================
// INIT
//====================================================

esp_err_t sen65_task_init(void)
{
    //------------------------------------------------
    // QUEUE
    //------------------------------------------------

    sen65_queue =
        xQueueCreate(
            SENSOR_QUEUE_SIZE,
            sizeof(air_raw_data_t));

    if (sen65_queue == NULL) {

        ESP_LOGE(TAG,
                 "Erro criando queue");

        return ESP_FAIL;
    }

    //------------------------------------------------
    // SENSOR INIT
    //------------------------------------------------

    ESP_LOGI(TAG,
             "Inicializando SEN65");

    esp_err_t err =
        sen65_init();

    if (err != ESP_OK) {

        ESP_LOGE(TAG,
                 "Erro sen65_init");

        return err;
    }

    //------------------------------------------------
    // START MEASUREMENT
    //------------------------------------------------

    err = sen65_start();

    if (err != ESP_OK) {

        ESP_LOGE(TAG,
                 "Erro sen65_start");

        return err;
    }

    //------------------------------------------------
    // WARMUP
    //------------------------------------------------

    ESP_LOGI(TAG,
             "Warmup sensor...");

    vTaskDelay(
        pdMS_TO_TICKS(
            SENSOR_WARMUP_TIME_MS));

    ESP_LOGI(TAG,
             "Sensor pronto");

    return ESP_OK;
}

//====================================================
// GET QUEUE
//====================================================

QueueHandle_t sen65_get_queue(void)
{
    return sen65_queue;
}

//====================================================
// FAN CLEAN
//====================================================

esp_err_t sen65_fan_clean(void)
{
    ESP_LOGW(TAG,
             "Fan cleaning");

    //------------------------------------------------
    // Caso exista na sua lib:
    //------------------------------------------------
    // return sen65_start_fan_cleaning();

    return ESP_OK;
}

//====================================================
// TASK
//====================================================

void sen65_task(void *pvParameters)
{
    sen65_data_t raw;

    air_raw_data_t data;

    while (1) {

        //------------------------------------------------
        // READ SENSOR
        //------------------------------------------------

        esp_err_t err =
            sen65_read(&raw);

        if (err == ESP_OK) {

            //--------------------------------------------
            // CONVERT
            //--------------------------------------------

            data.pm25 =
                raw.pm2_5;

            data.voc =
                raw.voc_index;

            data.nox =
                raw.nox_index;

            //--------------------------------------------
            // SEND QUEUE
            //--------------------------------------------

            if (xQueueSend(
                    sen65_queue,
                    &data,
                    pdMS_TO_TICKS(10))
                != pdTRUE) {

                ESP_LOGW(TAG,
                         "Queue cheia");
            }

        } else {

            ESP_LOGE(TAG,
                     "Erro leitura");
        }

        //------------------------------------------------
        // DELAY
        //------------------------------------------------

        vTaskDelay(
            pdMS_TO_TICKS(
                SENSOR_READ_INTERVAL_MS));
    }
}