#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  EVENT_IDLE = 0,
  EVENT_SUSPECT,
  EVENT_ACTIVE,
  EVENT_DECAY
} event_state_t;

typedef struct {
  uint16_t pm1;
  uint16_t pm25;
  uint16_t pm4;
  uint16_t pm10;
  int16_t voc;
  int16_t nox;
  int16_t rh_ticks;
  int16_t temp_ticks;
  uint16_t co2_ppm;
  bool co2_valid;
} air_raw_data_t;

typedef struct {
  uint16_t pm1;
  uint16_t pm25;
  uint16_t pm4;
  uint16_t pm10;
  int16_t voc;
  int16_t nox;
  uint16_t pm25_base;
  int16_t voc_base;
  int16_t nox_base;
  uint16_t co2_base;
  int16_t pm25_spike;
  int16_t voc_spike;
  int16_t nox_spike;
  int16_t rh_ticks;
  int16_t temp_ticks;
  uint16_t co2_ppm;
  bool co2_valid;
} air_processed_data_t;

typedef struct {
  event_state_t state;
  uint32_t duration;
  uint16_t peak_pm25;
  int16_t peak_voc;
  int16_t peak_nox;
  bool voc_led_suspect;

  int16_t co2_ppm_per_min;
  bool flash_crowding;

  int16_t thr_pm_start;
  int16_t thr_voc_start;
  int16_t thr_pm_active;
  int16_t thr_voc_active;
  int16_t thr_pm_end;
  int16_t thr_voc_end;
  int16_t thr_pm_idle;
  int16_t thr_voc_idle;

  int16_t adapt_vape_voc_soft;
  int16_t adapt_vape_voc_med;
  int16_t adapt_vape_voc_high;

  int16_t decay_neg_dpm1;
  int16_t decay_neg_dpm25;
  int16_t decay_neg_dpm10;
  uint8_t decay_vape_shape;
  uint8_t decay_dust_shape;
} air_event_t;

#define AIR_ALERT_FLASH_CROWDING (1u << 0)

typedef struct {
  uint8_t fire_score;
  uint8_t vape_score;
  uint8_t cig_score;
  uint8_t perfume_score;
  uint8_t dust_score;
  uint8_t alert_flags;
} air_classification_t;

#endif
