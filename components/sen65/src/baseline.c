#include "baseline.h"
#include "app_config.h"

#include <stdint.h>

static uint32_t pm25_base;
static int32_t voc_base;
static int32_t nox_base;
static uint32_t co2_base;

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
