#ifndef PRJ_STATE_H
#define PRJ_STATE_H

#include <stdbool.h>

typedef enum {
  prj_state_wifi_connected = 0,
  prj_state_mqtt_connected = 1,
  prj_state_device_pairing = 2,
} prj_state_id_t;

void prj_state_set_state(prj_state_id_t id, bool value);
bool prj_state_get(prj_state_id_t id);

#endif
