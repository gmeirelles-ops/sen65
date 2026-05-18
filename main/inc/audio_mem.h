#ifndef AUDIO_MEM_H
#define AUDIO_MEM_H

#include <stdlib.h>
#include "esp_err.h"

#define audio_calloc(n, sz) calloc((n), (sz))
#define AUDIO_MEM_CHECK(tag, ptr, action)                                        \
  do {                                                                           \
    if ((ptr) == NULL) {                                                         \
      ESP_LOGE(tag, "out of memory");                                            \
      action;                                                                    \
    }                                                                            \
  } while (0)
#define AUDIO_NULL_CHECK(tag, ptr, action) AUDIO_MEM_CHECK(tag, ptr, action)

#endif
