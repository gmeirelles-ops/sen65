#ifndef AIR_AQI_H
#define AIR_AQI_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  int aqi;              /* 0–500, -1 se invalido */
  const char *category_pt;
  const char *category_en;
  const char *color_name; /* ex.: "green" — para dashboards/MQTT */
  const char *color_hex;  /* ex.: "#00E400" — EPA AirNow */
} air_aqi_info_t;

/** PM2.5 em µg/m³ → AQI EPA (interpolação linear por faixa). */
int air_aqi_pm25_from_ugm3(float pm25_ugm3);

/** PM10 em µg/m³ → AQI EPA. */
int air_aqi_pm10_from_ugm3(float pm10_ugm3);

/** Metadados (categoria + cor) para um valor de AQI ja calculado. */
void air_aqi_fill_info(int aqi, air_aqi_info_t *out);

/** Codigo 0–5 alinhado as faixas EPA (para MQTT/UI). */
int air_aqi_category_code(int aqi);

/** AQI geral = max(PM2.5, PM10); preenche *out. Retorna false se ambos invalidos. */
bool air_aqi_overall_from_readings(float pm25_ugm3, float pm10_ugm3,
                                   air_aqi_info_t *out);

#endif
