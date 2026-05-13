#ifndef BASELINE_H
#define BASELINE_H

#include "types.h"

void baseline_init(void);
void baseline_process(air_processed_data_t *data);
void baseline_pure_air_checkpoint(const air_processed_data_t *data,
                                   const air_event_t *event);

#endif
