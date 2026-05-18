#include "air_json.h"
#include "air_aqi.h"
#include "air_quality_level.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "types.h"

#define AIR_JSON_MIN_CAP 448u

static bool sen65_invalid(int16_t v) { return v == (int16_t)0x7FFF; }

static float json_float_or_neg1(float v) {
  if (!isfinite(v)) {
    return -1.0f;
  }
  return v;
}

int air_json_encode_reading(const air_processed_data_t *d, char *buf,
                            size_t cap) {
  if (d == NULL || buf == NULL || cap < AIR_JSON_MIN_CAP) {
    return -1;
  }

  float pm1 = json_float_or_neg1(
      (d->pm1 != 0xFFFFu) ? ((float)d->pm1 / 10.0f) : -1.0f);
  float pm25 = json_float_or_neg1(
      (d->pm25 != 0xFFFFu) ? ((float)d->pm25 / 10.0f) : -1.0f);
  float pm10 = json_float_or_neg1(
      (d->pm10 != 0xFFFFu) ? ((float)d->pm10 / 10.0f) : -1.0f);
  int voc = sen65_invalid(d->voc) ? -1 : (int)d->voc;
  int nox = sen65_invalid(d->nox) ? -1 : (int)d->nox;
  float temp_c = json_float_or_neg1(
      sen65_invalid(d->temp_ticks) ? -1.0f : ((float)d->temp_ticks / 200.0f));
  float rh_pct = json_float_or_neg1(
      sen65_invalid(d->rh_ticks) ? -1.0f : ((float)d->rh_ticks / 100.0f));
  int co2 = d->co2_valid ? (int)d->co2_ppm : -1;

  int aqi_pm25 = air_aqi_pm25_from_ugm3(pm25);
  int aqi_pm10 = air_aqi_pm10_from_ugm3(pm10);

  air_aqi_info_t aqi;
  if (!air_aqi_overall_from_readings(pm25, pm10, &aqi)) {
    air_aqi_fill_info(-1, &aqi);
    aqi.category_pt = air_quality_level_name_pt(air_quality_worst_level(d));
  }

  int codigo = air_aqi_category_code(aqi.aqi);
  if (codigo < 0) {
    codigo = (int)air_quality_worst_level(d);
  }

  int n = snprintf(
      buf, cap,
      "{\"pm1_ugm3\":%.1f,\"pm25_ugm3\":%.1f,\"pm10_ugm3\":%.1f,"
      "\"pm25_aqi\":%d,\"pm10_aqi\":%d,\"aqi\":%d,"
      "\"qualidade_ar\":\"%s\",\"codigo_qualidade\":%d,"
      "\"indice_voc\":%d,\"indice_nox\":%d,\"co2_ppm\":%d,\"co2_valido\":%s,"
      "\"temp_c\":%.1f,\"umidade_pct\":%.1f}",
      pm1, pm25, pm10, aqi_pm25, aqi_pm10, aqi.aqi, aqi.category_pt, codigo,
      voc, nox, co2, d->co2_valid ? "true" : "false", temp_c, rh_pct);
  if (n < 0 || (size_t)n >= cap) {
    return -1;
  }
  return n;
}
