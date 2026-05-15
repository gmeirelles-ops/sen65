#include "adaptive_stats.h"
#include "app_config.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

#define RING ADAPTIVE_IDLE_RING

static int16_t s_pm[RING];
static int16_t s_voc[RING];
static uint16_t s_w;
static uint16_t s_n;

void adaptive_stats_reset(void) {
  memset(s_pm, 0, sizeof(s_pm));
  memset(s_voc, 0, sizeof(s_voc));
  s_w = 0;
  s_n = 0;
}

void adaptive_stats_idle_push(int16_t pm25_spike, int16_t voc_spike) {
  s_pm[s_w] = pm25_spike;
  s_voc[s_w] = voc_spike;
  s_w = (uint16_t)((s_w + 1u) % RING);
  if (s_n < RING) {
    s_n++;
  }
}

static void ring_std(const int16_t *buf, uint16_t n, float *sigma) {
  if (n < ADAPTIVE_MIN_SAMPLES) {
    *sigma = 0.f;
    return;
  }
  int64_t sum = 0;
  for (uint16_t i = 0; i < n; i++) {
    sum += buf[i];
  }
  float mean = (float)sum / (float)n;
  float acc = 0.f;
  for (uint16_t i = 0; i < n; i++) {
    float d = (float)buf[i] - mean;
    acc += d * d;
  }
  *sigma = sqrtf(acc / (float)n);
}

static void snapshot_oldest_first(int16_t *dst_pm, int16_t *dst_voc,
                                  uint16_t n) {
  uint16_t start = (uint16_t)((s_w + RING - n) % RING);
  for (uint16_t i = 0; i < n; i++) {
    uint16_t j = (uint16_t)((start + i) % RING);
    dst_pm[i] = s_pm[j];
    dst_voc[i] = s_voc[j];
  }
}

static int16_t scale_thr(int32_t num, int32_t den, int16_t base) {
  if (den <= 0) {
    return base;
  }
  int32_t v = (base * num) / den;
  if (v < 1) {
    v = 1;
  }
  if (v > 2000) {
    v = 2000;
  }
  return (int16_t)v;
}

void adaptive_stats_fill_event_thresholds(
    int16_t *thr_pm_start, int16_t *thr_voc_start, int16_t *thr_pm_active,
    int16_t *thr_voc_active, int16_t *thr_pm_end, int16_t *thr_voc_end,
    int16_t *thr_pm_idle, int16_t *thr_voc_idle, int16_t *adapt_vape_soft,
    int16_t *adapt_vape_med, int16_t *adapt_vape_high) {
  int16_t pm_copy[RING];
  int16_t voc_copy[RING];
  uint16_t n = s_n;
  if (n == 0) {
    *thr_pm_start = EVENT_PM_START_THRESHOLD;
    *thr_voc_start = EVENT_VOC_START_THRESHOLD;
    *thr_pm_active = EVENT_PM_ACTIVE_THRESHOLD;
    *thr_voc_active = EVENT_VOC_ACTIVE_THRESHOLD;
    *thr_pm_end = EVENT_PM_END_THRESHOLD;
    *thr_voc_end = EVENT_VOC_END_THRESHOLD;
    *thr_pm_idle = EVENT_PM_IDLE_THRESHOLD;
    *thr_voc_idle = EVENT_VOC_IDLE_THRESHOLD;
    *adapt_vape_soft = VAPE_VOC_SPIKE_SOFT;
    *adapt_vape_med = VAPE_VOC_SPIKE_MED;
    *adapt_vape_high = VAPE_VOC_SPIKE_HIGH;
    return;
  }
  snapshot_oldest_first(pm_copy, voc_copy, n);
  float sigma_pm = 0.f;
  float sigma_voc = 0.f;
  ring_std(pm_copy, n, &sigma_pm);
  ring_std(voc_copy, n, &sigma_voc);

  int16_t pm_s = (int16_t)(ADAPT_PM_START_FLOOR + ADAPT_K_PM * sigma_pm);
  if (pm_s < EVENT_PM_START_THRESHOLD / 2) {
    pm_s = EVENT_PM_START_THRESHOLD / 2;
  }
  if (pm_s < ADAPT_PM_START_FLOOR) {
    pm_s = ADAPT_PM_START_FLOOR;
  }

  int16_t voc_s = (int16_t)(ADAPT_VOC_START_FLOOR + ADAPT_K_VOC * sigma_voc);
  if (voc_s < EVENT_VOC_START_THRESHOLD / 2) {
    voc_s = EVENT_VOC_START_THRESHOLD / 2;
  }
  if (voc_s < ADAPT_VOC_START_FLOOR) {
    voc_s = ADAPT_VOC_START_FLOOR;
  }

  *thr_pm_start = pm_s;
  *thr_voc_start = voc_s;
  *thr_pm_active = scale_thr(ADAPT_PM_ACTIVE_SCALE_NUM,
                             ADAPT_PM_ACTIVE_SCALE_DEN, pm_s);
  if (*thr_pm_active < EVENT_PM_ACTIVE_THRESHOLD / 3) {
    *thr_pm_active = EVENT_PM_ACTIVE_THRESHOLD / 3;
  }
  *thr_voc_active = scale_thr(ADAPT_VOC_ACTIVE_SCALE_NUM,
                             ADAPT_VOC_ACTIVE_SCALE_DEN, voc_s);
  if (*thr_voc_active < 2) {
    *thr_voc_active = 2;
  }
  *thr_pm_end = scale_thr(ADAPT_PM_END_SCALE_NUM, ADAPT_PM_END_SCALE_DEN, pm_s);
  if (*thr_pm_end < 4) {
    *thr_pm_end = 4;
  }
  *thr_voc_end = scale_thr(ADAPT_VOC_END_SCALE_NUM, ADAPT_VOC_END_SCALE_DEN,
                           voc_s);
  if (*thr_voc_end < 1) {
    *thr_voc_end = 1;
  }
  *thr_pm_idle = scale_thr(ADAPT_PM_IDLE_SCALE_NUM, ADAPT_PM_IDLE_SCALE_DEN,
                           pm_s);
  if (*thr_pm_idle < 2) {
    *thr_pm_idle = 2;
  }
  *thr_voc_idle = scale_thr(ADAPT_VOC_IDLE_SCALE_NUM, ADAPT_VOC_IDLE_SCALE_DEN,
                            voc_s);
  if (*thr_voc_idle < 1) {
    *thr_voc_idle = 1;
  }

  int16_t vs = (int16_t)(ADAPT_VAPE_SOFT_BIAS + ADAPT_VAPE_SOFT_K * sigma_voc);
  if (vs < VAPE_VOC_SPIKE_SOFT / 2) {
    vs = VAPE_VOC_SPIKE_SOFT / 2;
  }
  *adapt_vape_soft = vs;

  int16_t vm = (int16_t)(ADAPT_VAPE_MED_BIAS + ADAPT_VAPE_MED_K * sigma_voc);
  if (vm <= *adapt_vape_soft + 8) {
    vm = (int16_t)(*adapt_vape_soft + 8);
  }
  *adapt_vape_med = vm;

  int16_t vh = (int16_t)(ADAPT_VAPE_HIGH_BIAS + ADAPT_VAPE_HIGH_K * sigma_voc);
  if (vh <= *adapt_vape_med + 20) {
    vh = (int16_t)(*adapt_vape_med + 20);
  }
  *adapt_vape_high = vh;
}
