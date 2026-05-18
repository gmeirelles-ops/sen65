#ifndef MAIN_H
#define MAIN_H

#include "app_config.h"
#include "esp_err.h"

#define MRM_ERROR_CHECK(x)                                                     \
  do {                                                                         \
    esp_err_t _err = (x);                                                      \
    if (_err != ESP_OK) {                                                      \
      ESP_LOGE("MRM", "%s:%d %s", __FILE__, __LINE__, esp_err_to_name(_err));   \
    }                                                                          \
  } while (0)

#define MQTT_TOPIC_BUFFER_SIZE 128
#define MQTT_PAYLOAD_BUFFER_SIZE 2048
#define MQTT_TASK_STACK_SIZE 6144

#define MRM_MQTT_URI MQTT_BROKER_URI
#define MRM_MQTT_PORT 0
#define MRM_MQTT_QOS MQTT_QOS_AIR
#define MRM_MQTT_KEEPALIVE 60

#define PRJ_VERSIONS "sen65-1.0"
#define DEVICE_PRODUCT_ID "SEN65"
#define SNTP_TZ_DEFAULT "UTC0"
#define PRJ_BOARD_OFFLINE_VOLUME 0

#define ACK_TOPIC_STR_FRM "%s/device/%s/ack"
#define CONFS_TOPIC_STR_FRM "%s/device/%s/confs"
#define STATUS_TOPIC_STR_FRM "%s/device/%s/status"
#define SDCARD_TOPIC_STR_FRM "%s/device/%s/sdcard"
#define DEVICE_RESET_TOPIC_STR_FRM "%s/device/%s/reset"
#define DEVICE_RUN_TOPIC_STR_FRM "%s/device/%s/run"
#define LIVE_TOPIC_STR_FRM "%s/live"
#define RADIO_TOPIC_STR_FRM "%s/radio"
#define PLAYLIST_TOPIC_STR_FRM "%s/playlist"
#define SENSOR_TOPIC_STR_FRM "%s/sensor"
#define TIMERS_TOPIC_STR_FRM "%s/timers"
#define GC_SNTP_TOPIC_STR_FRM "global/sntp"
#define DOWNLOADS_TOPIC_STR_FRM "%s/downloads"
#define GLOBAL_RESET_TOPIC_STR_FRM "%s/reset"
#define GLOBAL_CONFS_TOPIC_STR_FRM "%s/confs"
#define GLOBAL_INPUTS_TOPIC_STR_FRM "%s/inputs"
#define LORA_DEVICES_TOPIC_STR_FRM "%s/lora"

#define JSON_FIELD_STATUS "status"
#define JSON_FIELD_OFFLINE "offline"
#define JSON_FIELD_FW_VERSION "fw_version"
#define JSON_FIELD_DEVICE_ID "device_id"
#define JSON_FIELD_GROUP_ID "group_id"
#define JSON_FIELD_BOARD_TYPE "board_type"
#define JSON_FIELD_BOARD_SPEAKER "speaker"
#define JSON_FIELD_BOARD_MASTER "master"
#define JSON_FIELD_BOARD_SLAVE "slave"
#define JSON_FIELD_TIMEZONE "timezone"
#define JSON_FIELD_VOLUME "volume"
#define JSON_FIELD_NOISE_GAIN "noise_gain"
#define JSON_FIELD_DEVICE_OWNER "device_owner"
#define JSON_FIELD_PRODUCT_ID "product_id"

void prj_main_restart(void);

#endif
