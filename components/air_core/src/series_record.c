#include "series_record.h"

#include "app_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#if SERIES_RECORD_ENABLE

static const char *TAG = "SERIES";

typedef struct {
  int64_t ts_us;
  uint16_t pm1;
  uint16_t pm25;
  uint16_t pm4;
  uint16_t pm10;
  int16_t voc;
  int16_t nox;
  int16_t rh_ticks;
  int16_t temp_ticks;
  uint16_t co2_ppm;
  uint8_t co2_valid;
  int16_t pm_sp;
  int16_t voc_sp;
  int16_t nox_sp;
  uint8_t event_state;
  uint32_t duration;
  uint8_t fire;
  uint8_t vape;
  uint8_t cig;
  uint8_t perfume;
  uint8_t dust;
  uint8_t scenario_tag;
} series_row_t;

static series_row_t s_ring[SERIES_RECORD_RING_CAPACITY];
static uint16_t s_write;
static uint16_t s_count;
static volatile uint8_t s_scenario_tag;
static uint32_t s_sample_seq;
static bool s_stream_header_done;

static uint8_t s_bb_post_left;
static uint16_t s_bb_anchor;
static bool s_bb_armed;
static uint8_t s_bb_prev_vape;

static uint16_t ring_oldest_index(void) {
  if (s_count == 0) {
    return 0;
  }
  return (uint16_t)(((uint32_t)s_write + SERIES_RECORD_RING_CAPACITY -
                     (uint32_t)s_count) %
                    SERIES_RECORD_RING_CAPACITY);
}

static void format_csv_line(char *out, size_t out_sz, const series_row_t *r) {
  snprintf(out, out_sz,
           "S,%" PRId64 ",%u,%u,%u,%u,%d,%d,%d,%d,%u,%u,%d,%d,%d,%u,%" PRIu32
           ",%u,%u,%u,%u,%u,%u",
           r->ts_us, (unsigned)r->pm1, (unsigned)r->pm25, (unsigned)r->pm4,
           (unsigned)r->pm10, (int)r->voc, (int)r->nox, (int)r->rh_ticks,
           (int)r->temp_ticks, (unsigned)r->co2_ppm, (unsigned)r->co2_valid,
           (int)r->pm_sp, (int)r->voc_sp, (int)r->nox_sp,
           (unsigned)r->event_state, r->duration, (unsigned)r->fire,
           (unsigned)r->vape, (unsigned)r->cig, (unsigned)r->perfume,
           (unsigned)r->dust, (unsigned)r->scenario_tag);
}

static void blackbox_emit_json(uint16_t anchor) {
  const int pre = SERIES_BLACKBOX_PRE_SEC;
  const int post = SERIES_BLACKBOX_POST_SEC;
  const int n = pre + 1 + post;

  int prev_pm = 0;
  int prev_voc = 0;
  int prev_co2 = 0;
  bool have_prev = false;
  int64_t t0 = 0;

  int batch_pm[32];
  int batch_voc[32];
  int batch_co2[32];
  int batch_n = 0;
  int batch_start_i = 1;

  ESP_LOGW(TAG, "BLACKBOX_JSON_BEGIN");

  for (int i = 0; i < n; i++) {
    int off = -pre + i;
    uint16_t ix = (uint16_t)((anchor + off + SERIES_RECORD_RING_CAPACITY) %
                             SERIES_RECORD_RING_CAPACITY);
    const series_row_t *r = &s_ring[ix];
    int pm = (int)r->pm25;
    int vo = (int)r->voc;
    int co = (int)r->co2_ppm;
    int out_pm;
    int out_vo;
    int out_co;
    if (!have_prev) {
      t0 = r->ts_us;
      out_pm = pm;
      out_vo = vo;
      out_co = co;
      have_prev = true;
    } else {
      out_pm = pm - prev_pm;
      out_vo = vo - prev_voc;
      out_co = co - prev_co2;
    }
    prev_pm = pm;
    prev_voc = vo;
    prev_co2 = co;

    if (i == 0) {
      ESP_LOGW(TAG,
               "BB_HEAD:{\"pre\":%d,\"post\":%d,\"dt_ms\":1000,\"t0\":%" PRId64
               ",\"v0\":{\"pm25\":%d,\"voc\":%d,\"co2\":%d,\"co2v\":%u}}",
               pre, post, t0, out_pm, out_vo, out_co, (unsigned)r->co2_valid);
    } else {
      batch_pm[batch_n] = out_pm;
      batch_voc[batch_n] = out_vo;
      batch_co2[batch_n] = out_co;
      batch_n++;
    }

    if (batch_n == 32 || (i == n - 1 && batch_n > 0)) {
      char buf[480];
      size_t p = 0;
      p += (size_t)snprintf(buf + p, sizeof(buf) - p,
                            "BB_D{\"i0\":%d,\"n\":%d,\"pm\":[", batch_start_i,
                            batch_n);
      for (int k = 0; k < batch_n; k++) {
        p += (size_t)snprintf(buf + p, sizeof(buf) - p, "%s%d",
                              (k == 0) ? "" : ",", batch_pm[k]);
      }
      p += (size_t)snprintf(buf + p, sizeof(buf) - p, "],\"voc\":[");
      for (int k = 0; k < batch_n; k++) {
        p += (size_t)snprintf(buf + p, sizeof(buf) - p, "%s%d",
                              (k == 0) ? "" : ",", batch_voc[k]);
      }
      p += (size_t)snprintf(buf + p, sizeof(buf) - p, "],\"co2\":[");
      for (int k = 0; k < batch_n; k++) {
        p += (size_t)snprintf(buf + p, sizeof(buf) - p, "%s%d",
                              (k == 0) ? "" : ",", batch_co2[k]);
      }
      p += (size_t)snprintf(buf + p, sizeof(buf) - p, "]}");
      ESP_LOGW(TAG, "%s", buf);
      batch_start_i += batch_n;
      batch_n = 0;
    }
  }

  ESP_LOGW(TAG, "BLACKBOX_JSON_END");
}

static void blackbox_tick(uint8_t vape_score, uint16_t row_idx) {
  const uint8_t thr = DETECTION_SCORE_THRESHOLD;
  bool edge = (s_bb_prev_vape < thr && vape_score >= thr);
  s_bb_prev_vape = vape_score;

  if (edge && !s_bb_armed) {
    s_bb_armed = true;
    s_bb_anchor = row_idx;
    s_bb_post_left = (uint8_t)SERIES_BLACKBOX_POST_SEC;
    return;
  }
  if (!s_bb_armed) {
    return;
  }
  if (s_bb_post_left > 0) {
    s_bb_post_left--;
  }
  if (s_bb_post_left == 0) {
    blackbox_emit_json(s_bb_anchor);
    s_bb_armed = false;
  }
}

void series_record_init(void) {
  memset(s_ring, 0, sizeof(s_ring));
  s_write = 0;
  s_count = 0;
  s_scenario_tag = 0;
  s_sample_seq = 0;
  s_stream_header_done = false;
  s_bb_post_left = 0;
  s_bb_anchor = 0;
  s_bb_armed = false;
  s_bb_prev_vape = 0;
  ESP_LOGI(TAG, "Serie: anel %u | blackbox pre=%ds post=%ds",
           (unsigned)SERIES_RECORD_RING_CAPACITY, SERIES_BLACKBOX_PRE_SEC,
           SERIES_BLACKBOX_POST_SEC);
}

void series_record_set_scenario_tag(uint8_t tag) { s_scenario_tag = tag; }

uint8_t series_record_get_scenario_tag(void) { return s_scenario_tag; }

void series_record_sample(int64_t time_us, const air_processed_data_t *proc,
                          const air_event_t *evt,
                          const air_classification_t *cls) {
  series_row_t row = {
      .ts_us = time_us,
      .pm1 = proc->pm1,
      .pm25 = proc->pm25,
      .pm4 = proc->pm4,
      .pm10 = proc->pm10,
      .voc = proc->voc,
      .nox = proc->nox,
      .rh_ticks = proc->rh_ticks,
      .temp_ticks = proc->temp_ticks,
      .co2_ppm = proc->co2_ppm,
      .co2_valid = proc->co2_valid ? 1u : 0u,
      .pm_sp = proc->pm25_spike,
      .voc_sp = proc->voc_spike,
      .nox_sp = proc->nox_spike,
      .event_state = (uint8_t)evt->state,
      .duration = evt->duration,
      .fire = cls->fire_score,
      .vape = cls->vape_score,
      .cig = cls->cig_score,
      .perfume = cls->perfume_score,
      .dust = cls->dust_score,
      .scenario_tag = s_scenario_tag,
  };

  uint16_t row_idx = s_write;
  s_ring[s_write] = row;
  s_write = (uint16_t)((s_write + 1u) % SERIES_RECORD_RING_CAPACITY);
  if (s_count < SERIES_RECORD_RING_CAPACITY) {
    s_count++;
  }
  s_sample_seq++;

#if SERIES_STREAM_CSV_LINE
  char line[256];
  if (!s_stream_header_done) {
    ESP_LOGW("SERIES_CSV",
             "HEADER:ts_us,pm1,pm25,pm4,pm10,voc,nox,rh_ticks,temp_ticks,"
             "co2_ppm,co2_valid,pm_spike,voc_spike,nox_spike,evt_state,"
             "duration_ev,fire,vape,cig,perfume,dust,scenario_tag");
    s_stream_header_done = true;
  }
  format_csv_line(line, sizeof(line), &row);
  ESP_LOGI("SERIES_CSV", "%s", line);
#endif

  blackbox_tick(cls->vape_score, row_idx);

#if SERIES_AUTO_DUMP_INTERVAL_SAMPLES > 0
  if ((s_sample_seq % (uint32_t)SERIES_AUTO_DUMP_INTERVAL_SAMPLES) == 0u) {
    series_record_dump_ring_over_uart();
  }
#endif
}

void series_record_dump_ring_over_uart(void) {
  ESP_LOGW(TAG, "SERIES_DUMP_BEGIN,samples,%u", (unsigned)s_count);
  ESP_LOGW("SERIES_CSV",
           "HEADER:ts_us,pm1,pm25,pm4,pm10,voc,nox,rh_ticks,temp_ticks,"
           "co2_ppm,co2_valid,pm_spike,voc_spike,nox_spike,evt_state,"
           "duration_ev,fire,vape,cig,perfume,dust,scenario_tag");

  uint16_t n = s_count;
  uint16_t idx = ring_oldest_index();
  char line[256];
  uint16_t chunk = 0;

  for (uint16_t i = 0; i < n; i++) {
    format_csv_line(line, sizeof(line), &s_ring[idx]);
    ESP_LOGI("SERIES_CSV", "%s", line);
    idx = (uint16_t)((idx + 1u) % SERIES_RECORD_RING_CAPACITY);
    chunk++;
#if SERIES_AUTO_DUMP_CHUNK_LINES > 0
    if (chunk >= (uint16_t)SERIES_AUTO_DUMP_CHUNK_LINES) {
      chunk = 0;
      vTaskDelay(pdMS_TO_TICKS(20));
    }
#endif
  }
  ESP_LOGW(TAG, "SERIES_DUMP_END");
}

#else

void series_record_init(void) {}
void series_record_set_scenario_tag(uint8_t tag) { (void)tag; }
uint8_t series_record_get_scenario_tag(void) { return 0; }
void series_record_sample(int64_t time_us, const air_processed_data_t *proc,
                          const air_event_t *evt,
                          const air_classification_t *cls) {
  (void)time_us;
  (void)proc;
  (void)evt;
  (void)cls;
}
void series_record_dump_ring_over_uart(void) {}

#endif
