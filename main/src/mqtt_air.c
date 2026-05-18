#include "mqtt_air.h"

#include "app_config.h"
#include "esp_log.h"
#include "prj_mqtt.h"
#include "prj_network.h"
#include "prj_state.h"
#include "sdkconfig.h"

#if MQTT_AIR_ENABLE

#include "air_json.h"

static const char *TAG = "MQTT_AIR";

esp_err_t mqtt_air_init(void) {
  ESP_LOGI(TAG, "Rede: prj_confs (BluFi/JSON) + prj_wifi + prj_mqtt");
  return prj_network_init();
}

void mqtt_air_publish(const air_processed_data_t *processed) {
  if (processed == NULL || !prj_state_get(prj_state_mqtt_connected)) {
    return;
  }
  char buf[720];
  if (air_json_encode_reading(processed, buf, sizeof buf) < 0) {
    return;
  }
  prj_mqtt_publish_sensor(buf);
}

#else

esp_err_t mqtt_air_init(void) { return ESP_OK; }

void mqtt_air_publish(const air_processed_data_t *processed) {
  (void)processed;
}

#endif
