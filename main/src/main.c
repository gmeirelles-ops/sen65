#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"

#include "types.h"

#include "app_config.h"

#include "sen65_task.h"
#include "filter.h"
#include "baseline.h"
#include "event.h"
#include "classifier.h"
#include "telemetry.h"

static const char *TAG = "APP";

//====================================================
// PROCESS TASK
//====================================================

static void process_task(void *pvParameters)
{
    air_raw_data_t raw;

    air_processed_data_t processed;

    air_event_t event = {0};

    air_classification_t classification;

    QueueHandle_t queue =
        sen65_get_queue();

    while (1) {

        if (xQueueReceive(
                queue,
                &raw,
                portMAX_DELAY)
            == pdTRUE) {

            //----------------------------------------
            // FILTER
            //----------------------------------------

            filter_process(
                &raw,
                &processed);

            //----------------------------------------
            // BASELINE
            //----------------------------------------

            baseline_process(
                &processed);

            //----------------------------------------
            // EVENT
            //----------------------------------------

            event_process(
                &processed,
                &event);

            //----------------------------------------
            // CLASSIFIER
            //----------------------------------------

            classifier_process(
                &processed,
                &event,
                &classification);

            //----------------------------------------
            // TELEMETRY
            //----------------------------------------

            telemetry_log(
                &processed,
                &event,
                &classification);
        }
    }
}

//====================================================
// APP MAIN
//====================================================

void app_main(void)
{
    ESP_LOGI(TAG,
             "APP START");

    //------------------------------------------------
    // INIT SENSOR
    //------------------------------------------------

    if (sen65_task_init()
        != ESP_OK) {

        ESP_LOGE(TAG,
                 "Erro init sensor");

        return;
    }

    //------------------------------------------------
    // SENSOR TASK
    //------------------------------------------------

    xTaskCreate(

        sen65_task,

        "sen65_task",

        SENSOR_TASK_STACK,

        NULL,

        SENSOR_TASK_PRIORITY,

        NULL);

    //------------------------------------------------
    // PROCESS TASK
    //------------------------------------------------

    xTaskCreate(

        process_task,

        "process_task",

        PROCESS_TASK_STACK,

        NULL,

        PROCESS_TASK_PRIORITY,

        NULL);
}