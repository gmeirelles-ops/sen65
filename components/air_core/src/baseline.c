#include "baseline.h"
#include "app_config.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "nvs.h"

#include <stdint.h>
#include <string.h>

static const char *TAG = "BASELINE";

static uint32_t pm25_base;
static int32_t voc_base;
static int32_t nox_base;
static uint32_t co2_base;

#define NVS_NS "sen65_cal"
#define NVS_KEY_PURE "pure_v1"

typedef struct {
  uint32_t magic;
  int32_t voc_base_i32;
  int32_t nox_base_i32;
} baseline_pure_blob_t;

#define PURE_BLOB_MAGIC 0x50455231u

static int32_t pure_air_streak;
static int64_t pure_air_last_save_us;

void baseline_init(void) {
  nvs_handle_t h;
  esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &h);
  if (err != ESP_OK) {
    return;
  }
  baseline_pure_blob_t blob;
  memset(&blob, 0, sizeof(blob));
  size_t sz = sizeof(blob);
  err = nvs_get_blob(h, NVS_KEY_PURE, &blob, &sz);
  nvs_close(h);
  if (err != ESP_OK || sz != sizeof(blob) || blob.magic != PURE_BLOB_MAGIC) {
    return;
  }
  voc_base = blob.voc_base_i32;
  nox_base = blob.nox_base_i32;
  ESP_LOGI(TAG, "baseline VOC/NOx carregado da Flash");
}

void baseline_pure_air_checkpoint(const air_processed_data_t *data,
                                  const air_event_t *event) {
  if (event->state != EVENT_IDLE) {
    pure_air_streak = 0;
    return;
  }
  if (!data->co2_valid || data->co2_ppm < PURE_AIR_CO2_MIN_PPM ||
      data->co2_ppm > PURE_AIR_CO2_MAX_PPM) {
    pure_air_streak = 0;
    return;
  }
  if (data->pm25_spike > PURE_AIR_PM_SPIKE_MAX ||
      data->voc_spike > PURE_AIR_VOC_SPIKE_MAX ||
      data->nox_spike > PURE_AIR_NOX_SPIKE_MAX) {
    pure_air_streak = 0;
    return;
  }
  pure_air_streak++;
  if (pure_air_streak < (int32_t)PURE_AIR_STREAK_SAMPLES) {
    return;
  }
  int64_t now = (int64_t)esp_timer_get_time();
  if (pure_air_last_save_us != 0 &&
      (now - pure_air_last_save_us) < PURE_AIR_SAVE_MIN_INTERVAL_US) {
    return;
  }

  baseline_pure_blob_t blob = {.magic = PURE_BLOB_MAGIC,
                               .voc_base_i32 = voc_base,
                               .nox_base_i32 = nox_base};

  nvs_handle_t h;
  if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) {
    return;
  }
  esp_err_t w = nvs_set_blob(h, NVS_KEY_PURE, &blob, sizeof(blob));
  if (w == ESP_OK) {
    w = nvs_commit(h);
  }
  nvs_close(h);
  if (w == ESP_OK) {
    pure_air_last_save_us = now;
    pure_air_streak = 0;
    ESP_LOGI(TAG, "referencia ar puro salva (VOC/NOx baseline)");
  }
}

void baseline_process(air_processed_data_t *data) {
  pm25_base = ((uint32_t)data->pm25 * BASELINE_ALPHA +
               pm25_base * (BASELINE_DIV - BASELINE_ALPHA)) /
              BASELINE_DIV;

  voc_base = ((int32_t)data->voc * BASELINE_ALPHA +
              voc_base * (BASELINE_DIV - BASELINE_ALPHA)) /
             BASELINE_DIV;

  nox_base = ((int32_t)data->nox * BASELINE_ALPHA +
              nox_base * (BASELINE_DIV - BASELINE_ALPHA)) /
             BASELINE_DIV;

  data->pm25_base = (uint16_t)pm25_base;
  data->voc_base = (int16_t)voc_base;
  data->nox_base = (int16_t)nox_base;

  if (data->co2_valid) {
    co2_base = ((uint32_t)data->co2_ppm * BASELINE_ALPHA +
                co2_base * (BASELINE_DIV - BASELINE_ALPHA)) /
               BASELINE_DIV;
  }
  data->co2_base = (uint16_t)co2_base;

  data->pm25_spike = (int16_t)((int32_t)data->pm25 - (int32_t)pm25_base);
  data->voc_spike = (int16_t)((int32_t)data->voc - voc_base);
  data->nox_spike = (int16_t)((int32_t)data->nox - nox_base);

  if (data->pm25_spike < 0) {
    data->pm25_spike = 0;
  }
  if (data->voc_spike < 0) {
    data->voc_spike = 0;
  }
  if (data->nox_spike < 0) {
    data->nox_spike = 0;
  }

  if (data->pm25_spike < PM_NOISE_FLOOR) {
    data->pm25_spike = 0;
  }
  if (data->voc_spike < VOC_NOISE_FLOOR) {
    data->voc_spike = 0;
  }
  if (data->nox_spike < NOX_NOISE_FLOOR) {
    data->nox_spike = 0;
  }
}
