#ifndef SEN65_H
#define SEN65_H

#include "esp_err.h"

typedef struct {

    uint32_t pm1_0;
    uint32_t pm2_5;
    uint32_t pm4_0;
    uint32_t pm10;

    uint32_t humidity;
    uint32_t temperature;

    uint32_t voc_index;
    uint32_t nox_index;

} sen65_data_t;

esp_err_t sen65_init(void);

esp_err_t sen65_start(void);

esp_err_t sen65_read(sen65_data_t *data);

#endif