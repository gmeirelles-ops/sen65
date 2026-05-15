#include "telemetry.h"
#include "air_aqi.h"
#include "air_quality_level.h"
#include "app_config.h"
#include "esp_log.h"

static const char *TAG = "AIR";

static bool sen65_value_invalid_int16(int16_t v) { return v == (int16_t)0x7FFF; }

static bool s_prev_flash_crowding_log;

void telemetry_log(const air_processed_data_t *data, const air_event_t *event,
                   const air_classification_t *result) {
  
  // 1. DADOS AMBIENTAIS BÁSICOS (Impresso em linha única para scannability)
  float pm = (data->pm25 != 0xFFFFu) ? (data->pm25 / 10.0f) : -1.0f;
  float pm10 = (data->pm10 != 0xFFFFu) ? (data->pm10 / 10.0f) : -1.0f;
  int voc = !sen65_value_invalid_int16(data->voc) ? data->voc : -1;
  int nox = !sen65_value_invalid_int16(data->nox) ? data->nox : -1;
  int co2 = data->co2_valid ? (int)data->co2_ppm : -1;

  air_aqi_info_t aqi;
  if (!air_aqi_overall_from_readings(pm, pm10, &aqi)) {
    air_aqi_fill_info(-1, &aqi);
  }
  int aqi_pm25_only = air_aqi_pm25_from_ugm3(pm);

  // Converte temperatura para float rápido apenas para display
  float temp = !sen65_value_invalid_int16(data->temp_ticks) ? (data->temp_ticks / 200.0f) : -1.0f;
  float rh = !sen65_value_invalid_int16(data->rh_ticks) ? (data->rh_ticks / 100.0f) : -1.0f;

  // Log compacto de estado geral (métricas alinhadas a medidores portáteis)
  ESP_LOGI(TAG,
           "PM2.5: %.1f (AQI %d) | PM10: %.1f | AQI geral: %d | %s |  "
           "VOC(idx): %d | NOx(idx): %d | CO2: %d | T: %.1fC | RH: %.1f%%",
           pm, aqi_pm25_only >= 0 ? aqi_pm25_only : -1, pm10,
           aqi.aqi >= 0 ? aqi.aqi : -1, aqi.category_pt,  voc,
           nox, co2, temp, rh);

  // 2. DETALHES DO EVENTO (Só imprime se o ar não estiver 100% calmo)
  if (event->state > 0) { // 1=SUSPECT, 2=ACTIVE, 3=DECAY
    float ratio = (data->voc_spike > 1) ? ((float)data->pm25_spike / (float)data->voc_spike) : 99.0f;
    
    ESP_LOGW(TAG,
             "EVT [St:%d Dur:%lus pyro:%d] Spikes(PM:%d VOC:%d NOx:%d) r:%.2f "
             "dCO2/min:%d thrVOC:%d",
             event->state, (unsigned long)event->duration,
             (int)event->voc_led_suspect, data->pm25_spike, data->voc_spike,
             data->nox_spike, ratio, (int)event->co2_ppm_per_min,
             (int)event->adapt_vape_voc_soft);
    
    ESP_LOGD(TAG, "Scores -> Fire:%u Vape:%u Cig:%u Perf:%u Dust:%u",
             result->fire_score, result->vape_score, result->cig_score,
             result->perfume_score, result->dust_score);
  }

  {
    bool crowding = event->flash_crowding ||
                    (result->alert_flags & AIR_ALERT_FLASH_CROWDING) != 0;
    if (crowding && !s_prev_flash_crowding_log) {
      ESP_LOGW(TAG, ">>> FLASH_CROWDING (CO2 ~%d ppm/min suav.) <<<",
               (int)event->co2_ppm_per_min);
    }
    s_prev_flash_crowding_log = crowding;
  }

  // 3. ALERTAS CRÍTICOS (Gatilhos de Ação)
  float ratio_evt = (data->voc_spike > 1)
                        ? ((float)data->pm25_spike / (float)data->voc_spike)
                        : 99.0f;
  int16_t co2_spike_log = 0;
  if (data->co2_valid && data->co2_ppm > 400) {
    uint16_t base = (data->co2_base > 400) ? data->co2_base : 450;
    if (data->co2_ppm > base) {
      co2_spike_log = (int16_t)(data->co2_ppm - base);
    }
  }
  const bool prefer_cig_noxless =
      (data->nox_spike < CIG_NOX_SPIKE_SOFT) &&
      (data->pm25_spike >= CIG_NOXLESS_PM_SPIKE) &&
      (data->voc_spike >= CIG_NOXLESS_VOC_SPIKE) &&
      (ratio_evt >= CIG_NOXLESS_RATIO_MIN) &&
      ((!data->co2_valid) || (co2_spike_log >= CIG_NOXLESS_CO2_SPIKE));

  const bool prefer_cig_alert =
      (result->cig_score >= DETECTION_SCORE_THRESHOLD) &&
      (result->cig_score + CIG_ALERT_OVER_FIRE >= result->fire_score) &&
      (data->pm25_spike >= CIG_PM_SPIKE_SOFT) &&
      (data->voc_spike >= CIG_VOC_SPIKE_SOFT) &&
      ((data->nox_spike >= CIG_NOX_SPIKE_SOFT) || prefer_cig_noxless);

  if (prefer_cig_alert) {
    ESP_LOGW(TAG, ">>> ALERTA: CIGARRO DETECTADO <<<");
  } else if (result->fire_score >= FIRE_SCORE_THRESHOLD) {
    ESP_LOGE(TAG, ">>> ALERTA INCENDIO <<<");
  } else if (result->vape_score >= DETECTION_SCORE_THRESHOLD) {
    ESP_LOGW(TAG, ">>> ALERTA: VAPE DETECTADO <<<");
  } else if (result->cig_score >= DETECTION_SCORE_THRESHOLD) {
    ESP_LOGW(TAG, ">>> ALERTA: CIGARRO DETECTADO <<<");
  }
}