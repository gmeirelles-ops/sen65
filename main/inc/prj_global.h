#ifndef PRJ_GLOBAL_H
#define PRJ_GLOBAL_H

#include "main.h"
#include "mqtt_client.h"

typedef struct {
  uint8_t mqtt_device_id[24];
  uint8_t mqtt_ack_topic[MQTT_TOPIC_BUFFER_SIZE];
  uint8_t mqtt_device_topic_confs[MQTT_TOPIC_BUFFER_SIZE];
  uint8_t mqtt_device_topic_status[MQTT_TOPIC_BUFFER_SIZE];
  uint8_t mqtt_device_topic_sdcard[MQTT_TOPIC_BUFFER_SIZE];
  uint8_t mqtt_device_topic_reset[MQTT_TOPIC_BUFFER_SIZE];
  uint8_t mqtt_device_topic_run[MQTT_TOPIC_BUFFER_SIZE];
  uint8_t mqtt_live_topic[MQTT_TOPIC_BUFFER_SIZE];
  uint8_t mqtt_radio_topic[MQTT_TOPIC_BUFFER_SIZE];
  uint8_t mqtt_playlist_topic[MQTT_TOPIC_BUFFER_SIZE];
  uint8_t mqtt_sensor_topic[MQTT_TOPIC_BUFFER_SIZE];
  uint8_t mqtt_timers_topic[MQTT_TOPIC_BUFFER_SIZE];
  uint8_t mqtt_sntp_topic[MQTT_TOPIC_BUFFER_SIZE];
  uint8_t mqtt_downloads_topic[MQTT_TOPIC_BUFFER_SIZE];
  uint8_t mqtt_reset_topic[MQTT_TOPIC_BUFFER_SIZE];
  uint8_t mqtt_confs_topic[MQTT_TOPIC_BUFFER_SIZE];
  uint8_t mqtt_inputs_topic[MQTT_TOPIC_BUFFER_SIZE];
  uint8_t mqtt_device_lora_topic[MQTT_TOPIC_BUFFER_SIZE];
  char lan_ip[16];
  int volume;
  int noise_gain;
  int group_id;
  esp_mqtt_client_config_t mqtt_client_config;
  esp_mqtt_client_handle_t mqtt_client_handle;
} prj_global_instance_t;

esp_err_t prj_global_init(void);
prj_global_instance_t *prj_global_instance_get_data(void);
void prj_global_wait_take(void);
void prj_global_wait_give(void);
void prj_global_strrmv(char *str, char ch, size_t max_len);

#endif
