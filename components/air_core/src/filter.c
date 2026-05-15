#include "filter.h"
#include "app_config.h"

#include <stdint.h>

static uint32_t pm1_f;
static uint32_t pm25_f;
static uint32_t pm4_f;
static uint32_t pm10_f;
static int32_t voc_f;
static int32_t nox_f;

void filter_process(const air_raw_data_t *input, air_processed_data_t *output) {
  pm1_f = ((uint32_t)input->pm1 * EMA_FAST_ALPHA +
           pm1_f * (EMA_FAST_DIV - EMA_FAST_ALPHA)) /
          EMA_FAST_DIV;

  pm25_f = ((uint32_t)input->pm25 * EMA_FAST_ALPHA +
            pm25_f * (EMA_FAST_DIV - EMA_FAST_ALPHA)) /
           EMA_FAST_DIV;

  pm4_f = ((uint32_t)input->pm4 * EMA_FAST_ALPHA +
           pm4_f * (EMA_FAST_DIV - EMA_FAST_ALPHA)) /
          EMA_FAST_DIV;

  pm10_f = ((uint32_t)input->pm10 * EMA_FAST_ALPHA +
            pm10_f * (EMA_FAST_DIV - EMA_FAST_ALPHA)) /
           EMA_FAST_DIV;

  voc_f = ((int32_t)input->voc * EMA_FAST_ALPHA +
           voc_f * (EMA_FAST_DIV - EMA_FAST_ALPHA)) /
          EMA_FAST_DIV;

  nox_f = ((int32_t)input->nox * EMA_FAST_ALPHA +
           nox_f * (EMA_FAST_DIV - EMA_FAST_ALPHA)) /
          EMA_FAST_DIV;

  output->pm1 = (uint16_t)pm1_f;
  output->pm25 = (uint16_t)pm25_f;
  output->pm4 = (uint16_t)pm4_f;
  output->pm10 = (uint16_t)pm10_f;
  output->voc = (int16_t)voc_f;
  output->nox = (int16_t)nox_f;
  output->rh_ticks = input->rh_ticks;
  output->temp_ticks = input->temp_ticks;
  output->co2_ppm = input->co2_ppm;
  output->co2_valid = input->co2_valid;
}
