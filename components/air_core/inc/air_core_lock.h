#ifndef AIR_CORE_LOCK_H
#define AIR_CORE_LOCK_H

#include "esp_err.h"

/** Mutex do pipeline (filter/baseline/event/adaptive_stats/series_record). */
esp_err_t air_core_lock_init(void);

void air_core_lock(void);
void air_core_unlock(void);

#endif
