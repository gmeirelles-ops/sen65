#ifndef FILTER_H
#define FILTER_H

#include "types.h"

void filter_process(const air_raw_data_t *input, air_processed_data_t *output);

#endif
