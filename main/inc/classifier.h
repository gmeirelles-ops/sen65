#ifndef CLASSIFIER_H
#define CLASSIFIER_H

#include "types.h"

//====================================================
// CLASSIFIER
//====================================================

void classifier_process(

    const air_processed_data_t *data,

    const air_event_t *event,

    air_classification_t *result);

#endif