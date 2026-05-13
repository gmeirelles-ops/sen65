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
  uint16_t pm25;
  int16_t voc;
  int16_t nox;
  int16_t rh_ticks;
  int16_t temp_ticks;
  uint16_t co2_ppm;
  bool co2_valid;
} air_raw_data_t;

typedef struct {
  uint16_t pm25;
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
} air_event_t;

typedef struct {
  uint8_t fire_score;
  uint8_t vape_score;
  uint8_t cig_score;
  uint8_t perfume_score;
  uint8_t dust_score;
} air_classification_t;

#endif
