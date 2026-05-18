#include "air_core_lock.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static SemaphoreHandle_t s_mtx;

esp_err_t air_core_lock_init(void) {
  if (s_mtx != NULL) {
    return ESP_OK;
  }
  s_mtx = xSemaphoreCreateMutex();
  return (s_mtx != NULL) ? ESP_OK : ESP_ERR_NO_MEM;
}

void air_core_lock(void) {
  if (s_mtx != NULL) {
    (void)xSemaphoreTake(s_mtx, portMAX_DELAY);
  }
}

void air_core_unlock(void) {
  if (s_mtx != NULL) {
    (void)xSemaphoreGive(s_mtx);
  }
}
