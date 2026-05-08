#include "filter.h"

#include "app_config.h"

//====================================================
// FILTERED VALUES
//====================================================

static uint16_t pm25_f = 0;
static uint16_t voc_f  = 0;
static uint16_t nox_f  = 0;

//====================================================
// FILTER PROCESS
//====================================================

void filter_process(

    const air_raw_data_t *input,

    air_processed_data_t *output)
{
    //------------------------------------------------
    // PM2.5 EMA
    //------------------------------------------------

    pm25_f =
        ((input->pm25 * EMA_FAST_ALPHA) +

        (pm25_f *
        (EMA_FAST_DIV - EMA_FAST_ALPHA)))

        / EMA_FAST_DIV;

    //------------------------------------------------
    // VOC EMA
    //------------------------------------------------

    voc_f =
        ((input->voc * EMA_FAST_ALPHA) +

        (voc_f *
        (EMA_FAST_DIV - EMA_FAST_ALPHA)))

        / EMA_FAST_DIV;

    //------------------------------------------------
    // NOX EMA
    //------------------------------------------------

    nox_f =
        ((input->nox * EMA_FAST_ALPHA) +

        (nox_f *
        (EMA_FAST_DIV - EMA_FAST_ALPHA)))

        / EMA_FAST_DIV;

    //------------------------------------------------
    // OUTPUT
    //------------------------------------------------

    output->pm25 = pm25_f;

    output->voc  = voc_f;

    output->nox  = nox_f;
}