#include "event.h"
#include "adaptive_stats.h"
#include "app_config.h"

void event_process(const air_processed_data_t *data, air_event_t *event) {
  static uint16_t prev_pm1;
  static uint16_t prev_pm25;
  static uint16_t prev_pm10;
  static uint16_t prev_co2;
  static bool have_prev_pm;
  static bool have_prev_co2;

  if (event->state == EVENT_IDLE) {
    adaptive_stats_idle_push(data->pm25_spike, data->voc_spike);
  }

  adaptive_stats_fill_event_thresholds(
      &event->thr_pm_start, &event->thr_voc_start, &event->thr_pm_active,
      &event->thr_voc_active, &event->thr_pm_end, &event->thr_voc_end,
      &event->thr_pm_idle, &event->thr_voc_idle, &event->adapt_vape_voc_soft,
      &event->adapt_vape_voc_med, &event->adapt_vape_voc_high);

  event->co2_ppm_per_min = 0;
  event->flash_crowding = false;
  if (data->co2_valid && have_prev_co2) {
    static uint32_t co2_smooth;
    static uint16_t co2_smooth_prev;
    static bool have_co2_smooth_prev;
    static uint8_t crowd_streak;

    uint32_t ppm = (uint32_t)data->co2_ppm;
    if (!have_co2_smooth_prev) {
      co2_smooth = ppm * CROWD_CO2_EMA_DIV;
      co2_smooth_prev = (uint16_t)ppm;
      have_co2_smooth_prev = true;
      crowd_streak = 0;
    } else {
      co2_smooth = (co2_smooth * (CROWD_CO2_EMA_DIV - CROWD_CO2_EMA_ALPHA) +
                     ppm * CROWD_CO2_EMA_ALPHA) /
                    CROWD_CO2_EMA_DIV;
    }
    uint16_t co2_s = (uint16_t)(co2_smooth / CROWD_CO2_EMA_DIV);

    int32_t d = (int32_t)co2_s - (int32_t)co2_smooth_prev;
    co2_smooth_prev = co2_s;

    int32_t ppm_min = d * 60;
    if (ppm_min > 32767) {
      ppm_min = 32767;
    }
    if (ppm_min < -32768) {
      ppm_min = -32768;
    }
    event->co2_ppm_per_min = (int16_t)ppm_min;

    if (ppm_min >= (int32_t)CROWD_CO2_DERIV_PPM_PER_MIN &&
        data->co2_ppm >= CROWD_CO2_MIN_PPM) {
      if (crowd_streak < 255) {
        crowd_streak++;
      }
    } else {
      crowd_streak = 0;
    }

    if (crowd_streak >= CROWD_CO2_STREAK_SAMPLES) {
      event->flash_crowding = true;
    }
  }

  event->decay_neg_dpm1 = 0;
  event->decay_neg_dpm25 = 0;
  event->decay_neg_dpm10 = 0;
  event->decay_vape_shape = 0;
  event->decay_dust_shape = 0;

  if (event->state == EVENT_DECAY && have_prev_pm) {
    int32_t d1 = (int32_t)data->pm1 - (int32_t)prev_pm1;
    int32_t d25 = (int32_t)data->pm25 - (int32_t)prev_pm25;
    int32_t d10 = (int32_t)data->pm10 - (int32_t)prev_pm10;
    int16_t n1 = (d1 < 0) ? (int16_t)(-d1) : 0;
    int16_t n25 = (d25 < 0) ? (int16_t)(-d25) : 0;
    int16_t n10 = (d10 < 0) ? (int16_t)(-d10) : 0;
    event->decay_neg_dpm1 = n1;
    event->decay_neg_dpm25 = n25;
    event->decay_neg_dpm10 = n10;

    if (n1 >= DECAY_DROP_MIN_MAG && n25 >= DECAY_DROP_MIN_MAG) {
      int32_t diff = (int32_t)n1 - (int32_t)n25;
      if (diff < 0) {
        diff = -diff;
      }
      uint32_t pen = (uint32_t)diff * 5u;
      if (pen > 100u) {
        pen = 100u;
      }
      event->decay_vape_shape = (uint8_t)(100u - pen);
    }

    if (n1 > 0 && n10 > (int32_t)n1 * DECAY_DUST_PM10_VS_PM1_NUM /
                         DECAY_DUST_PM10_VS_PM1_DEN &&
        n10 >= DECAY_DROP_MIN_MAG) {
      event->decay_dust_shape = 100;
    } else if (n10 > n1 + DECAY_DROP_MIN_MAG && n10 >= DECAY_DROP_MIN_MAG) {
      event->decay_dust_shape = 55;
    }
  }

  switch (event->state) {
    case EVENT_IDLE:
      if (data->pm25_spike > event->thr_pm_start ||
          data->voc_spike > event->thr_voc_start) {
        event->voc_led_suspect =
            (data->voc_spike > event->thr_voc_start) &&
            (data->pm25_spike <= event->thr_pm_start);
        event->state = EVENT_SUSPECT;
        event->duration = 0;
      }
      break;

    case EVENT_SUSPECT:
      event->duration++;
      if (data->pm25_spike > event->thr_pm_active ||
          data->voc_spike > event->thr_voc_active) {
        event->state = EVENT_ACTIVE;
      }
      if (data->pm25_spike < event->thr_pm_end &&
          data->voc_spike < event->thr_voc_end) {
        event->voc_led_suspect = false;
        event->state = EVENT_IDLE;
      }
      break;

    case EVENT_ACTIVE:
      event->duration++;
      if (data->pm25 > event->peak_pm25) {
        event->peak_pm25 = data->pm25;
      }
      if (data->voc > event->peak_voc) {
        event->peak_voc = data->voc;
      }
      if (data->nox > event->peak_nox) {
        event->peak_nox = data->nox;
      }
      if (data->pm25_spike < event->thr_pm_end &&
          data->voc_spike < event->thr_voc_end) {
        event->state = EVENT_DECAY;
      }
      break;

    case EVENT_DECAY:
      if (data->pm25_spike < event->thr_pm_idle &&
          data->voc_spike < event->thr_voc_idle) {
        event->voc_led_suspect = false;
        event->state = EVENT_IDLE;
      }
      break;
  }

  prev_pm1 = data->pm1;
  prev_pm25 = data->pm25;
  prev_pm10 = data->pm10;
  have_prev_pm = true;
  if (data->co2_valid) {
    prev_co2 = data->co2_ppm;
    have_prev_co2 = true;
  }
}
