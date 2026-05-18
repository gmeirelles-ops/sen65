#include "prj_global.h"

#include <string.h>
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static prj_global_instance_t s_instance;
static SemaphoreHandle_t s_boot_sem;

esp_err_t prj_global_init(void) {
  memset(&s_instance, 0, sizeof(s_instance));
  s_instance.volume = PRJ_BOARD_OFFLINE_VOLUME;
  if (s_boot_sem == NULL) {
    s_boot_sem = xSemaphoreCreateBinary();
    if (s_boot_sem == NULL) {
      return ESP_ERR_NO_MEM;
    }
    /* Inicia vazio: wait_take bloqueia ate wait_give (Wi-Fi/MQTT/pareamento). */
  }
  return ESP_OK;
}

prj_global_instance_t *prj_global_instance_get_data(void) { return &s_instance; }

void prj_global_wait_take(void) {
  if (s_boot_sem != NULL) {
    xSemaphoreTake(s_boot_sem, portMAX_DELAY);
  }
}

void prj_global_wait_give(void) {
  if (s_boot_sem != NULL) {
    xSemaphoreGive(s_boot_sem);
  }
}

void prj_global_strrmv(char *str, char ch, size_t max_len) {
  if (str == NULL || max_len == 0) {
    return;
  }
  size_t w = 0;
  for (size_t r = 0; str[r] != '\0' && r < max_len; r++) {
    if (str[r] != ch) {
      str[w++] = str[r];
    }
  }
  if (w < max_len) {
    str[w] = '\0';
  } else if (max_len > 0) {
    str[max_len - 1] = '\0';
  }
}

void prj_main_restart(void) {
  esp_restart();
}
