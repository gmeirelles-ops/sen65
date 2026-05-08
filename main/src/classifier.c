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

    //------------------------------------------------
    // RATIO
    //------------------------------------------------

    float pm_voc_ratio = 0.0f;

    if (data->voc_spike > 0.1f)
    {
        pm_voc_ratio =
            data->pm25_spike /
            data->voc_spike;
    }

   //------------------------------------------------
// VAPE
//------------------------------------------------

result->vape_score = 0;

// VOC alto
if (data->voc_spike > VAPE_VOC_THRESHOLD)
    result->vape_score += 20;

// NOX baixo
if (data->nox < VAPE_NOX_MAX)
    result->vape_score += 10;

// PM realmente relevante
if (data->pm25_spike > 8)
    result->vape_score += 25;

if (data->pm25_spike > 15)
    result->vape_score += 15;

// Relação PM/VOC típica de aerosol
if (pm_voc_ratio > 1)
    result->vape_score += 20;

if (pm_voc_ratio > 2)
    result->vape_score += 10;

// Evento sustentado
if (event->duration > 10)
    result->vape_score += 10;

// Penalização forte para gás/isqueiro
if (data->voc_spike > 100 &&
    data->pm25_spike < 6)
{
    result->vape_score -= 50;
}

// Penalização para VOC absurdo com PM baixo
if (data->voc_spike > 150 &&
    data->pm25_spike < 10)
{
    result->vape_score -= 30;
}

if (result->vape_score < 0)
    result->vape_score = 0;
    //------------------------------------------------
    // CIGARRO
    //------------------------------------------------

    if (data->pm25_spike >
        CIG_PM_THRESHOLD)
    {
        result->cig_score += 20;
    }

    if (data->voc_spike >
        CIG_VOC_THRESHOLD)
    {
        result->cig_score += 25;
    }

    // cigarro tende a NOX maior
    if (data->nox >
        CIG_NOX_THRESHOLD)
    {
        result->cig_score += 35;
    }

    // fumaça mais persistente
    if (event->duration > 10)
    {
        result->cig_score += 20;
    }

    //------------------------------------------------
    // PERFUME
    //------------------------------------------------

    if (

        data->voc_spike >
        PERFUME_VOC_THRESHOLD

        &&

        data->pm25_spike <
        PERFUME_PM_MAX

        &&

        data->nox_spike <
        10

        &&

        event->duration <
        15
    )
    {
        result->perfume_score = 80;
    }
    else
    {
        result->perfume_score = 0;
    }

    //------------------------------------------------
    // POEIRA
    //------------------------------------------------

    // Poeira:
    // PM alto + VOC baixo

    if (data->pm25_spike >
        DUST_PM_THRESHOLD

        &&

        data->voc_spike <
        DUST_VOC_MAX)
    {
        result->dust_score = 80;
    }
    else
    {
        result->dust_score = 0;
    }

    //------------------------------------------------
    // CLAMP
    //------------------------------------------------

    if (result->vape_score < 0)
        result->vape_score = 0;

    if (result->vape_score > 100)
        result->vape_score = 100;

    if (result->cig_score > 100)
        result->cig_score = 100;
}