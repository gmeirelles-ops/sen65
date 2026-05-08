#include "classifier.h"
#include "app_config.h"

//====================================================
// CLASSIFIER
//====================================================

void classifier_process(
    const air_processed_data_t *data,
    const air_event_t *event,
    air_classification_t *result)
{
    //------------------------------------------------
    // RESET
    //------------------------------------------------

    result->vape_score    = 0;
    result->cig_score     = 0;
    result->perfume_score = 0;
    result->dust_score    = 0;

    // Usamos variáveis temporárias com sinal para permitir cálculos negativos
    int16_t temp_vape_score = 0;
    int16_t temp_cig_score  = 0;

    //------------------------------------------------
    // RATIO
    //------------------------------------------------

    float pm_voc_ratio = 0.0f;

    if (data->voc_spike > 0.1f) {
        pm_voc_ratio = (float)data->pm25_spike / (float)data->voc_spike;
    }

    //------------------------------------------------
    // VAPE LOGIC - REVISADA
    //------------------------------------------------

    temp_vape_score = 0;

    // Se o PM for muito baixo, ignoramos quase tudo.
    // Vape sem particulado não existe.
    if (data->pm25_spike < 10) {
        temp_vape_score -= 40; 
    }

    // VOC alto ganha pontos, mas só se tiver PM acompanhando
    if (data->voc_spike > VAPE_VOC_THRESHOLD) {
        temp_vape_score += 15;
    }

    // NOx baixo é típico (vape não queima nada)
    if (data->nox < VAPE_NOX_MAX) {
        temp_vape_score += 5;
    }

    // Pontuação baseada em Particulados (Aumentamos o rigor aqui)
    if (data->pm25_spike > 15) {
        temp_vape_score += 30;
    }
    if (data->pm25_spike > 30) {
        temp_vape_score += 20;
    }

    // Relação PM/VOC (Vape real tem MUITO PM para o tanto de VOC)
    // Se o ratio for < 0.5 (como no seu log 2.8 / 128 = 0.02), penaliza pesado.
    if (pm_voc_ratio < 0.2f) {
        temp_vape_score -= 60; // Penalização monstro
    } else if (pm_voc_ratio > 1.0f) {
        temp_vape_score += 20;
    }

    // Evento sustentado
    if (event->duration > 10) {
        temp_vape_score += 10;
    }

    //------------------------------------------------
    // CIGARRO LOGIC
    //------------------------------------------------

    if (data->pm25_spike > CIG_PM_THRESHOLD) {
        temp_cig_score += 20;
    }

    if (data->voc_spike > CIG_VOC_THRESHOLD) {
        temp_cig_score += 25;
    }

    // Cigarro tende a gerar NOX por causa da combustão
    if (data->nox > CIG_NOX_THRESHOLD) {
        temp_cig_score += 35;
    }

    // Fumaça de cigarro é persistente
    if (event->duration > 10) {
        temp_cig_score += 20;
    }

    //------------------------------------------------
    // PERFUME
    //------------------------------------------------

    if (data->voc_spike > PERFUME_VOC_THRESHOLD &&
        data->pm25_spike < PERFUME_PM_MAX &&
        data->nox_spike < 10 &&
        event->duration < 15) 
    {
        result->perfume_score = 80;
    } else {
        result->perfume_score = 0;
    }

    //------------------------------------------------
    // POEIRA
    //------------------------------------------------

    if (data->pm25_spike > DUST_PM_THRESHOLD &&
        data->voc_spike < DUST_VOC_MAX) 
    {
        result->dust_score = 80;
    } else {
        result->dust_score = 0;
    }

    //------------------------------------------------
    // FINAL CLAMP & ASSIGNMENT
    //------------------------------------------------

    // Proteção contra valores negativos antes de converter para uint8_t
    if (temp_vape_score < 0) {
        temp_vape_score = 0;
    }
    if (temp_vape_score > 100) {
        temp_vape_score = 100;
    }
    result->vape_score = (uint8_t)temp_vape_score;

    if (temp_cig_score < 0) {
        temp_cig_score = 0;
    }
    if (temp_cig_score > 100) {
        temp_cig_score = 100;
    }
    result->cig_score = (uint8_t)temp_cig_score;
}