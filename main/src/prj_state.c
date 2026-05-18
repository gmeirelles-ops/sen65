#include "prj_state.h"

static bool s_wifi;
static bool s_mqtt;
static bool s_pairing;

void prj_state_set_state(prj_state_id_t id, bool value) {
  switch (id) {
  case prj_state_wifi_connected:
    s_wifi = value;
    break;
  case prj_state_mqtt_connected:
    s_mqtt = value;
    break;
  case prj_state_device_pairing:
    s_pairing = value;
    break;
  }
}

bool prj_state_get(prj_state_id_t id) {
  switch (id) {
  case prj_state_wifi_connected:
    return s_wifi;
  case prj_state_mqtt_connected:
    return s_mqtt;
  case prj_state_device_pairing:
    return s_pairing;
  default:
    return false;
  }
}
