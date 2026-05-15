#include "air_quality_level.h"
#include "app_config.h"

#include <stdint.h>

static air_ui_level_t level_from_pm25_ugm3(float ugm3) {
  if (ugm3 < 0.f) {
    return AIR_UI_GOOD;
  }
  if (ugm3 <= (float)AIR_UI_PM25_GOOD_MAX) {
    return AIR_UI_GOOD;
  }
  if (ugm3 <= (float)AIR_UI_PM25_SLIGHT_MAX) {
    return AIR_UI_SLIGHT;
  }
  if (ugm3 <= (float)AIR_UI_PM25_MODERATE_MAX) {
    return AIR_UI_MODERATE;
  }
  return AIR_UI_SERIOUS;
}

static air_ui_level_t level_from_pm10_ugm3(float ugm3) {
  if (ugm3 < 0.f) {
    return AIR_UI_GOOD;
  }
  if (ugm3 <= (float)AIR_UI_PM10_GOOD_MAX) {
    return AIR_UI_GOOD;
  }
  if (ugm3 <= (float)AIR_UI_PM10_SLIGHT_MAX) {
    return AIR_UI_SLIGHT;
  }
  if (ugm3 <= (float)AIR_UI_PM10_MODERATE_MAX) {
    return AIR_UI_MODERATE;
  }
  return AIR_UI_SERIOUS;
}

static air_ui_level_t level_from_co2(uint16_t ppm, bool valid) {
  if (!valid) {
    return AIR_UI_GOOD;
  }
  if (ppm <= AIR_UI_CO2_GOOD_MAX) {
    return AIR_UI_GOOD;
  }
  if (ppm <= AIR_UI_CO2_SLIGHT_MAX) {
    return AIR_UI_SLIGHT;
  }
  if (ppm <= AIR_UI_CO2_MODERATE_MAX) {
    return AIR_UI_MODERATE;
  }
  return AIR_UI_SERIOUS;
}

air_ui_level_t air_quality_worst_level(const air_processed_data_t *data) {
  float pm25 =
      (data->pm25 != 0xFFFFu) ? ((float)data->pm25 / 10.0f) : -1.0f;
  float pm10 =
      (data->pm10 != 0xFFFFu) ? ((float)data->pm10 / 10.0f) : -1.0f;
  air_ui_level_t l25 = level_from_pm25_ugm3(pm25);
  air_ui_level_t l10 = level_from_pm10_ugm3(pm10);
  air_ui_level_t lc = level_from_co2(data->co2_ppm, data->co2_valid);
  air_ui_level_t m = l25;
  if (l10 > m) {
    m = l10;
  }
  if (lc > m) {
    m = lc;
  }
  return m;
}

const char *air_quality_level_name_pt(air_ui_level_t level) {
  switch (level) {
  case AIR_UI_GOOD:
    return "Bom";
  case AIR_UI_SLIGHT:
    return "Leve";
  case AIR_UI_MODERATE:
    return "Moderado";
  case AIR_UI_SERIOUS:
    return "Grave";
  default:
    return "?";
  }
}
