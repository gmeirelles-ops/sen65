#ifndef PRJ_DATA_H
#define PRJ_DATA_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#define PRJ_STA_SSID_MAX 32
#define PRJ_STA_PASS_MAX 64
#define PRJ_CLIENT_ID_MAX 48
#define PRJ_MQTT_USER_MAX 64
#define PRJ_MQTT_PASS_MAX 64

typedef enum {
  auth_type_personal = 0,
  auth_type_enterprise = 1,
} prj_auth_type_t;

typedef enum {
  eap_type_wpa2 = 0,
  eap_type_wpa3 = 1,
} prj_eap_type_t;

typedef enum {
  eap_method_ttls = 0,
} prj_eap_method_t;

typedef enum {
  prj_board_type_speaker = 0,
  prj_board_type_mrm_master = 1,
  prj_board_type_slave = 2,
  prj_board_type_sensor = 3,
} prj_board_type_t;

typedef struct {
  char ssid[PRJ_STA_SSID_MAX + 1];
  char password[PRJ_STA_PASS_MAX + 1];
  uint8_t bssid[6];
} prj_sta_t;

typedef struct {
  prj_auth_type_t auth_type;
  prj_eap_type_t eap_type;
  prj_eap_method_t eap_method;
  int eap_ttls_phase2_method;
  char eap_ca[256];
  char eap_id[64];
  char eap_username[64];
  char eap_password[64];
} prj_autentication_t;

typedef struct {
  char client_id[PRJ_CLIENT_ID_MAX + 1];
} prj_client_t;

typedef struct {
  char username[PRJ_MQTT_USER_MAX + 1];
  char password[PRJ_MQTT_PASS_MAX + 1];
} prj_mqtt_cred_t;

typedef struct {
  bool is_configured;
  bool is_to_pair;
  prj_board_type_t prj_board_type;
} prj_device_t;

typedef struct {
  prj_sta_t sta;
  prj_autentication_t autentication;
  prj_client_t client;
  prj_mqtt_cred_t mqtt;
  prj_device_t device;
} prj_data_t;

void prj_data_init(void);
prj_data_t *prj_data_load(void);
esp_err_t prj_data_save(void);
void prj_data_set_sta(const char *ssid, const char *password);
bool prj_data_has_wifi(void);

#endif
