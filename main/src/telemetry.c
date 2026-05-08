#include "telemetry.h"

#include "esp_log.h"

#include "app_config.h"

static const char *TAG = "TELEMETRY";

//====================================================
// LOG
//====================================================

void telemetry_log(

    const air_processed_data_t *data,

    const air_event_t *event,

    const air_classification_t *result)
{
    ESP_LOGI(TAG,
             "======================");

    //------------------------------------------------
    // RAW DATA
    //------------------------------------------------

    ESP_LOGI(TAG,
             "PM: %u.%u",
             data->pm25 / 10,
             data->pm25 % 10);

    ESP_LOGI(TAG,
             "VOC: %u.%u",
             data->voc / 10,
             data->voc % 10);

    ESP_LOGI(TAG,
             "NOX: %u.%u",
             data->nox / 10,
             data->nox % 10);

    //------------------------------------------------
    // BASELINES
    //------------------------------------------------

    ESP_LOGI(TAG,
             "PM_BASE: %u.%u",
             data->pm25_base / 10,
             data->pm25_base % 10);

    ESP_LOGI(TAG,
             "VOC_BASE: %u.%u",
             data->voc_base / 10,
             data->voc_base % 10);

    ESP_LOGI(TAG,
             "NOX_BASE: %u.%u",
             data->nox_base / 10,
             data->nox_base % 10);

    //------------------------------------------------
    // SPIKES
    //------------------------------------------------

    ESP_LOGI(TAG,
             "PM_SPIKE: %u.%u",
             data->pm25_spike / 10,
             data->pm25_spike % 10);

    ESP_LOGI(TAG,
             "VOC_SPIKE: %u.%u",
             data->voc_spike / 10,
             data->voc_spike % 10);

    ESP_LOGI(TAG,
             "NOX_SPIKE: %u.%u",
             data->nox_spike / 10,
             data->nox_spike % 10);

    //------------------------------------------------
    // EVENT
    //------------------------------------------------

    ESP_LOGI(TAG,
             "STATE: %d",
             event->state);

    ESP_LOGI(TAG,
             "DURATION: %lu",
             (unsigned long)event->duration);

    //------------------------------------------------
    // CLASSIFIER INPUTS
    //------------------------------------------------

    ESP_LOGI(TAG,
             "EVENT_ACTIVE: %s",

             event->state == EVENT_ACTIVE
             ? "YES"
             : "NO");

    ESP_LOGI(TAG,
             "PM_HIGH: %s",

             data->pm25_spike > 50
             ? "YES"
             : "NO");

    ESP_LOGI(TAG,
             "VOC_HIGH: %s",

             data->voc_spike > 50
             ? "YES"
             : "NO");

    ESP_LOGI(TAG,
             "LOW_NOX: %s",

             data->nox < 40
             ? "YES"
             : "NO");

    //------------------------------------------------
    // SCORES
    //------------------------------------------------

    ESP_LOGI(TAG,
             "VAPE_SCORE: %u",
             result->vape_score);

    ESP_LOGI(TAG,
             "CIG_SCORE: %u",
             result->cig_score);

    ESP_LOGI(TAG,
             "PERFUME_SCORE: %u",
             result->perfume_score);

    ESP_LOGI(TAG,
             "DUST_SCORE: %u",
             result->dust_score);

    //------------------------------------------------
    // DETECTION
    //------------------------------------------------

    if (result->vape_score >=
        DETECTION_SCORE_THRESHOLD) {

        ESP_LOGW(TAG,
                 "🚬 VAPE DETECTADO");
    }

    else if (result->cig_score >=
             DETECTION_SCORE_THRESHOLD) {

        ESP_LOGW(TAG,
                 "🚬 CIGARRO DETECTADO");
    }

    else if (result->perfume_score >=
             DETECTION_SCORE_THRESHOLD) {

        ESP_LOGW(TAG,
                 "🧴 PERFUME DETECTADO");
    }

    else if (result->dust_score >=
             DETECTION_SCORE_THRESHOLD) {

        ESP_LOGW(TAG,
                 "🌫️ POEIRA DETECTADA");
    }

    //------------------------------------------------
    // END
    //------------------------------------------------

    ESP_LOGI(TAG,
             "======================");
}