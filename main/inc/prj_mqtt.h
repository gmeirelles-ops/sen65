#ifndef PRJ_MQTT_H
#define PRJ_MQTT_H

#include "main.h"
#include "prj_global.h"

typedef struct {
  uint8_t topic[MQTT_TOPIC_BUFFER_SIZE];
  uint8_t payload[MQTT_PAYLOAD_BUFFER_SIZE];
  int msgid;
  int retain;
} prj_mqtt_data_t;

esp_err_t prj_mqtt_init(prj_global_instance_t *prj_global_instance);
esp_err_t prj_mqtt_send(prj_mqtt_data_t *prj_mqtt_data);
esp_err_t prj_mqtt_receive(prj_mqtt_data_t *prj_mqtt_data);
esp_err_t prj_mqtt_subscribe_all(void);
void prj_mqtt_publish_confs(void);
void prj_mqtt_publish_pair(void);
void prj_mqtt_publish_sensor(const char *json_payload);

#endif
