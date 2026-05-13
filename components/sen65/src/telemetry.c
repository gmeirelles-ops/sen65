#include "telemetry.h"
#include "app_config.h"
#include "esp_log.h"

static const char *TAG = "TELEMETRY";

static bool sen65_value_invalid_int16(int16_t v) { return v == (int16_t)0x7FFF; }

void telemetry_log(const air_processed_data_t *data, const air_event_t *event,
                   const air_classification_t *result) {
  ESP_LOGI(TAG, "========== AIR ==========");

  if (data->pm25 != 0xFFFFu) {
    ESP_LOGI(TAG,
             "PM2.5:%u (3) => %.1f ug/m3 ",
             (unsigned)data->pm25, data->pm25 / 10.0f);
  } else {
    ESP_LOGI(TAG, "PM2.5: desconhecido (0xFFFF)");
  }

  if (!sen65_value_invalid_int16(data->voc)) {
    ESP_LOGI(TAG,
             "VOC indice %d ",
             (int)data->voc);
  } else {
    ESP_LOGI(TAG, "VOC: invalido (indice 0x7FFF)");
  }

  if (!sen65_value_invalid_int16(data->nox)) {
    ESP_LOGI(TAG,
             "NOx indice %d ",
             (int)data->nox);
  } else {
    ESP_LOGI(TAG, "NOx: invalido (indice 0x7FFF)");
  }

  if (!sen65_value_invalid_int16(data->temp_ticks)) {
    int32_t tc = (int32_t)data->temp_ticks * 1000 / 200;
    bool neg = tc < 0;
    if (neg) {
      tc = -tc;
    }
    ESP_LOGI(TAG,
             "Temperatura: ticks=%d => %s%ld.%03ld C [datasheet: T = ticks/200]",
             (int)data->temp_ticks, neg ? "-" : "", (long)(tc / 1000),
             (long)(tc % 1000));
  } else {
    ESP_LOGI(TAG, "Temperatura: invalido");
  }

  if (!sen65_value_invalid_int16(data->rh_ticks)) {
    int32_t rh = (int32_t)data->rh_ticks;
    if (rh < 0) {
      rh = 0;
    }
    int32_t inteiro = rh / 100;
    int32_t cent = rh % 100;
    if (cent < 0) {
      cent = -cent;
    }
    ESP_LOGI(TAG,
             "Umidade: ticks=%d => %ld.%02ld %%RH [datasheet: %%RH = ticks/100]",
             (int)data->rh_ticks, (long)inteiro, (long)cent);
  } else {
    ESP_LOGI(TAG, "Umidade: invalido");
  }

  if (data->co2_valid) {
    ESP_LOGI(TAG, "CO2 (ACD1200): %u ppm", (unsigned)data->co2_ppm);
  } else {
    ESP_LOGI(TAG, "CO2: sem leitura valida (pre-aquecimento ACD1200 / erro)");
  }

  ESP_LOGI(TAG, "--- Baseline ---");
  ESP_LOGI(TAG, "PM_base: %u | VOC_base: %d | NOx_base: %d",
           (unsigned)data->pm25_base, (int)data->voc_base, (int)data->nox_base);

  ESP_LOGI(TAG, "--- Picos ---");
  ESP_LOGI(TAG, "PM_spike: %d | VOC_spike: %d | NOx_spike: %d",
           (int)data->pm25_spike, (int)data->voc_spike, (int)data->nox_spike);

  ESP_LOGI(TAG, "--- Evento ---");
  ESP_LOGI(TAG, "Estado: %d (0=IDLE 1=SUSPECT 2=ACTIVE 3=DECAY)",
           (int)event->state);
  ESP_LOGI(TAG, "Duracao (~s @1Hz): %lu", (unsigned long)event->duration);
  ESP_LOGI(TAG, "Picos evento PM / VOC / NOx: %u / %d / %d",
           (unsigned)event->peak_pm25, (int)event->peak_voc, (int)event->peak_nox);
           // Evita divisão por zero
float ratio = (data->voc_spike > 1) ? ((float)data->pm25_spike / (float)data->voc_spike) : 99.0f;
ESP_LOGI(TAG, "Razao PM/VOC (r): %.2f", ratio);

  ESP_LOGI(TAG, "--- Scores (heuristica) ---");
  ESP_LOGI(TAG, "INCENDIO: %u (limiar %u)", (unsigned)result->fire_score,
           (unsigned)FIRE_SCORE_THRESHOLD);
  ESP_LOGI(TAG, "VAPE: %u (limiar %u)", (unsigned)result->vape_score,
           (unsigned)DETECTION_SCORE_THRESHOLD);
  ESP_LOGI(TAG, "CIGARRO: %u", (unsigned)result->cig_score);
  ESP_LOGI(TAG, "PERFUME: %u | POEIRA: %u", (unsigned)result->perfume_score,
           (unsigned)result->dust_score);

  if (result->fire_score >= FIRE_SCORE_THRESHOLD) {
    ESP_LOGW(TAG, ">>> ALERTA INCENDIO (heuristica) <<<");
  } else if (result->vape_score >= DETECTION_SCORE_THRESHOLD) {
    ESP_LOGW(TAG, ">>> Indicio VAPE <<<");
  } else if (result->cig_score >= DETECTION_SCORE_THRESHOLD) {
    ESP_LOGW(TAG, ">>> Indicio CIGARRO <<<");
  } else if (result->perfume_score >= DETECTION_SCORE_THRESHOLD) {
    ESP_LOGW(TAG, ">>> Indicio PERFUME <<<");
  } else if (result->dust_score >= DETECTION_SCORE_THRESHOLD) {
    ESP_LOGW(TAG, ">>> Indicio POEIRA <<<");
  }

  ESP_LOGI(TAG, "===========================");
}
