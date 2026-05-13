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
  uint16_t pm25;
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
           "S,%" PRId64 ",%u,%d,%d,%d,%d,%u,%u,%d,%d,%d,%u,%" PRIu32
           ",%u,%u,%u,%u,%u,%u",
           r->ts_us, (unsigned)r->pm25, (int)r->voc, (int)r->nox,
           (int)r->rh_ticks, (int)r->temp_ticks, (unsigned)r->co2_ppm,
           (unsigned)r->co2_valid, (int)r->pm_sp, (int)r->voc_sp,
           (int)r->nox_sp, (unsigned)r->event_state, r->duration,
           (unsigned)r->fire, (unsigned)r->vape, (unsigned)r->cig,
           (unsigned)r->perfume, (unsigned)r->dust, (unsigned)r->scenario_tag);
}

void series_record_init(void) {
  memset(s_ring, 0, sizeof(s_ring));
  s_write = 0;
  s_count = 0;
  s_scenario_tag = 0;
  s_sample_seq = 0;
  s_stream_header_done = false;
  ESP_LOGI(TAG, "Serie CSV: anel %u linhas (tag SERIES_CSV)",
           (unsigned)SERIES_RECORD_RING_CAPACITY);
}

void series_record_set_scenario_tag(uint8_t tag) { s_scenario_tag = tag; }

uint8_t series_record_get_scenario_tag(void) { return s_scenario_tag; }

void series_record_sample(int64_t time_us, const air_processed_data_t *proc,
                          const air_event_t *evt,
                          const air_classification_t *cls) {
  series_row_t row = {
      .ts_us = time_us,
      .pm25 = proc->pm25,
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

  s_ring[s_write] = row;
  s_write = (uint16_t)((s_write + 1u) % SERIES_RECORD_RING_CAPACITY);
  if (s_count < SERIES_RECORD_RING_CAPACITY) {
    s_count++;
  }
  s_sample_seq++;

#if SERIES_STREAM_CSV_LINE
  char line[192];
  if (!s_stream_header_done) {
    ESP_LOGW("SERIES_CSV",
             "HEADER:ts_us,pm25,voc,nox,rh_ticks,temp_ticks,co2_ppm,co2_valid,"
             "pm_spike,voc_spike,nox_spike,evt_state,duration_ev,"
             "fire,vape,cig,perfume,dust,scenario_tag");
    s_stream_header_done = true;
  }
  format_csv_line(line, sizeof(line), &row);
  ESP_LOGI("SERIES_CSV", "%s", line);
#endif

#if SERIES_AUTO_DUMP_INTERVAL_SAMPLES > 0
  if ((s_sample_seq % (uint32_t)SERIES_AUTO_DUMP_INTERVAL_SAMPLES) == 0u) {
    series_record_dump_ring_over_uart();
  }
#endif
}

void series_record_dump_ring_over_uart(void) {
  ESP_LOGW(TAG, "SERIES_DUMP_BEGIN,samples,%u", (unsigned)s_count);
  ESP_LOGW("SERIES_CSV",
           "HEADER:ts_us,pm25,voc,nox,rh_ticks,temp_ticks,co2_ppm,co2_valid,"
           "pm_spike,voc_spike,nox_spike,evt_state,duration_ev,"
           "fire,vape,cig,perfume,dust,scenario_tag");

  uint16_t n = s_count;
  uint16_t idx = ring_oldest_index();
  char line[192];
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
