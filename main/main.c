#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "acd1200.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#include "app_config.h"
#include "baseline.h"
#include "classifier.h"
#include "event.h"
#include "filter.h"
#include "i2c_board.h"
#include "sen65_task.h"
#include "series_record.h"
#include "telemetry.h"
#include "types.h"

static const char *TAG = "APP";

static void process_task(void *pvParameters) {
  (void)pvParameters;
  air_raw_data_t raw;
  air_processed_data_t processed;
  air_event_t event = {0};
  air_classification_t classification;
  QueueHandle_t queue = sen65_get_queue();

  while (1) {
    if (xQueueReceive(queue, &raw, portMAX_DELAY) == pdTRUE) {
      filter_process(&raw, &processed);
      baseline_process(&processed);
      event_process(&processed, &event);
      baseline_pure_air_checkpoint(&processed, &event);
      classifier_process(&processed, &event, &classification);
      //series_record_sample(esp_timer_get_time(), &processed, &event,
        //                   &classification);
      telemetry_log(&processed, &event, &classification);
    }
  }
}

void app_main(void) {
  ESP_LOGI(TAG, "Inicio — qualidade do ar ");

  esp_err_t nvs_err = nvs_flash_init();
  if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES ||
      nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    nvs_err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(nvs_err);
  baseline_init();

  //series_record_init();

  i2c_master_bus_handle_t bus = NULL;
  ESP_ERROR_CHECK(board_i2c_init(&bus));
  ESP_ERROR_CHECK(acd1200_init(bus));
  ESP_ERROR_CHECK(sen65_task_init(bus));

  xTaskCreate(acd1200_task, "acd1200_task", ACD1200_TASK_STACK, NULL,
              ACD1200_TASK_PRIORITY, NULL);
  xTaskCreate(sen65_task, "sen65_task", SENSOR_TASK_STACK, NULL,
              SENSOR_TASK_PRIORITY, NULL);
  xTaskCreate(process_task, "process_task", PROCESS_TASK_STACK, NULL,
              PROCESS_TASK_PRIORITY, NULL);
}
