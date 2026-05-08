#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>

//====================================================
// EVENT STATES
//====================================================

typedef enum {

    EVENT_IDLE = 0,
    EVENT_SUSPECT,
    EVENT_ACTIVE,
    EVENT_DECAY

} event_state_t;

//====================================================
// SENSOR RAW
//====================================================

typedef struct {

    uint16_t pm25;
    uint16_t voc;
    uint16_t nox;

} air_raw_data_t;

//====================================================
// FILTERED
//====================================================

typedef struct {

    uint16_t pm25;
    uint16_t voc;
    uint16_t nox;

    uint16_t pm25_base;
    uint16_t voc_base;
    uint16_t nox_base;

    int16_t pm25_spike;
    int16_t voc_spike;
    int16_t nox_spike;

} air_processed_data_t;

//====================================================
// EVENT
//====================================================

typedef struct {

    event_state_t state;

    uint32_t duration;

    uint16_t peak_pm25;
    uint16_t peak_voc;
    uint16_t peak_nox;

} air_event_t;

//====================================================
// CLASSIFICATION
//====================================================

typedef struct {

    uint8_t vape_score;
    uint8_t cig_score;
    uint8_t perfume_score;
    uint8_t dust_score;

} air_classification_t;

#endif