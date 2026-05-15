#ifndef AIR_JSON_H
#define AIR_JSON_H

#include <stddef.h>

#include "types.h"

/** JSON UTF-8 com leituras + nível qualitativo (uma linha). Retorna bytes escritos ou <0. */
int air_json_encode_reading(const air_processed_data_t *processed, char *buf,
                            size_t cap);

#endif
