#ifndef SERIES_RECORD_H
#define SERIES_RECORD_H

#include <stdint.h>

#include "types.h"

void series_record_init(void);
void series_record_set_scenario_tag(uint8_t tag);
uint8_t series_record_get_scenario_tag(void);
void series_record_sample(int64_t time_us, const air_processed_data_t *proc,
                          const air_event_t *evt,
                          const air_classification_t *cls);
void series_record_dump_ring_over_uart(void);

#endif
