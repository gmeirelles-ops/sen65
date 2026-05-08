#include "baseline.h"

#include "app_config.h"

//====================================================
// BASELINES
//====================================================

static uint16_t pm25_base = 0;
static uint16_t voc_base  = 0;
static uint16_t nox_base  = 0;

//====================================================
// PROCESS
//====================================================

void baseline_process(
    air_processed_data_t *data)
{
    //------------------------------------------------
    // PM BASELINE
    //------------------------------------------------

    pm25_base =

        ((data->pm25 * BASELINE_ALPHA) +

        (pm25_base *
        (BASELINE_DIV - BASELINE_ALPHA)))

        / BASELINE_DIV;

    //------------------------------------------------
    // VOC BASELINE
    //------------------------------------------------

    voc_base =

        ((data->voc * BASELINE_ALPHA) +

        (voc_base *
        (BASELINE_DIV - BASELINE_ALPHA)))

        / BASELINE_DIV;

    //------------------------------------------------
    // NOX BASELINE
    //------------------------------------------------

    nox_base =

        ((data->nox * BASELINE_ALPHA) +

        (nox_base *
        (BASELINE_DIV - BASELINE_ALPHA)))

        / BASELINE_DIV;

    //------------------------------------------------
    // STORE
    //------------------------------------------------

    data->pm25_base = pm25_base;
    data->voc_base  = voc_base;
    data->nox_base  = nox_base;

    //------------------------------------------------
    // SPIKES
    //------------------------------------------------

    data->pm25_spike =
        data->pm25 - pm25_base;

    data->voc_spike =
        data->voc - voc_base;

    data->nox_spike =
        data->nox - nox_base;

    //------------------------------------------------
    // NEGATIVE CLAMP
    //------------------------------------------------

    if (data->pm25_spike < 0)
        data->pm25_spike = 0;

    if (data->voc_spike < 0)
        data->voc_spike = 0;

    if (data->nox_spike < 0)
        data->nox_spike = 0;

    //------------------------------------------------
    // NOISE FLOOR
    //------------------------------------------------

    if (data->pm25_spike < PM_NOISE_FLOOR)
        data->pm25_spike = 0;

    if (data->voc_spike < VOC_NOISE_FLOOR)
        data->voc_spike = 0;

    if (data->nox_spike < NOX_NOISE_FLOOR)
        data->nox_spike = 0;
}