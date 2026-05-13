#ifndef TELEMETRY_H
#define TELEMETRY_H

#include "types.h"

void telemetry_log(const air_processed_data_t *data, const air_event_t *event,
                   const air_classification_t *result);

#endif
