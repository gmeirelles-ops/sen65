#include "telemetry.h"
#include "app_config.h"
#include "esp_log.h"

static const char *TAG = "AIR";

static bool sen65_value_invalid_int16(int16_t v) { return v == (int16_t)0x7FFF; }

void telemetry_log(const air_processed_data_t *data, const air_event_t *event,
                   const air_classification_t *result) {
  
  // 1. DADOS AMBIENTAIS BÁSICOS (Impresso em linha única para scannability)
  float pm = (data->pm25 != 0xFFFFu) ? (data->pm25 / 10.0f) : -1.0f;
  int voc = !sen65_value_invalid_int16(data->voc) ? data->voc : -1;
  int nox = !sen65_value_invalid_int16(data->nox) ? data->nox : -1;
  int co2 = data->co2_valid ? (int)data->co2_ppm : -1;
  
  // Converte temperatura para float rápido apenas para display
  float temp = !sen65_value_invalid_int16(data->temp_ticks) ? (data->temp_ticks / 200.0f) : -1.0f;
  float rh = !sen65_value_invalid_int16(data->rh_ticks) ? (data->rh_ticks / 100.0f) : -1.0f;

  // Log compacto de estado geral
  ESP_LOGI(TAG, "PM2.5: %.1f | VOC: %d | NOx: %d | CO2: %d | T: %.1fC | RH: %.1f%%",
           pm, voc, nox, co2, temp, rh);

  // 2. DETALHES DO EVENTO (Só imprime se o ar não estiver 100% calmo)
  if (event->state > 0) { // 1=SUSPECT, 2=ACTIVE, 3=DECAY
    float ratio = (data->voc_spike > 1) ? ((float)data->pm25_spike / (float)data->voc_spike) : 99.0f;
    
    ESP_LOGW(TAG,
             "EVT [St:%d Dur:%lus pyro:%d] Spikes(PM:%d VOC:%d NOx:%d) r:%.2f",
             event->state, (unsigned long)event->duration,
             (int)event->voc_led_suspect, data->pm25_spike, data->voc_spike,
             data->nox_spike, ratio);
    
    ESP_LOGD(TAG, "Scores -> Fire:%u Vape:%u Cig:%u Perf:%u Dust:%u",
             result->fire_score, result->vape_score, result->cig_score,
             result->perfume_score, result->dust_score);
  }

  // 3. ALERTAS CRÍTICOS (Gatilhos de Ação)
  if (result->fire_score >= FIRE_SCORE_THRESHOLD) {
    ESP_LOGE(TAG, ">>> ALERTA INCENDIO <<<");
  } else if (result->vape_score >= DETECTION_SCORE_THRESHOLD) {
    ESP_LOGW(TAG, ">>> ALERTA: VAPE DETECTADO <<<");
  } else if (result->cig_score >= DETECTION_SCORE_THRESHOLD) {
    ESP_LOGW(TAG, ">>> ALERTA: CIGARRO DETECTADO <<<");
  }
}