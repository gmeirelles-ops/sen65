#ifndef AIR_QUALITY_LEVEL_H
#define AIR_QUALITY_LEVEL_H

#include "types.h"

typedef enum {
  AIR_UI_GOOD = 0,
  AIR_UI_SLIGHT,
  AIR_UI_MODERATE,
  AIR_UI_SERIOUS,
} air_ui_level_t;

air_ui_level_t air_quality_worst_level(const air_processed_data_t *data);

const char *air_quality_level_name_pt(air_ui_level_t level);

#endif
