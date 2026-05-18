#include "prj_confs.h"

#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "prj_data.h"
#include "prj_defines.h"
#include "prj_global.h"
#include "prj_state.h"
#include "wifi_blufi.h"

static const char *TAG = "prj_confs";
static TimerHandle_t s_pair_timer;
static bool s_boot_window_active;
static prj_confs_exit_t s_exit;

static esp_err_t pairing_save_nvs(void) {
  esp_err_t err = prj_data_save();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "NVS save falhou: %s", esp_err_to_name(err));
  }
  return err;
}

static void pairing_leave_mode(bool configured) {
  prj_data_t *d = prj_data_load();
  d->device.is_to_pair = false;
  if (configured) {
    d->device.is_configured = true;
  }
  pairing_save_nvs();
}

static void pairing_finish(bool timeout) {
  if (!s_boot_window_active) {
    return;
  }
  s_boot_window_active = false;

  if (timeout) {
    ESP_LOGW(TAG, "Janela inicial (%d ms) terminada — BluFi continua ativo",
             PRJ_PAIR_TIMEOUT);
    pairing_leave_mode(true);
    s_exit = PRJ_CONFS_EXIT_TIMEOUT;
  }

  if (s_pair_timer != NULL) {
    xTimerStop(s_pair_timer, 0);
  }

  prj_state_set_state(prj_state_device_pairing, false);
  prj_global_wait_give();
}

static void pair_timer_cb(TimerHandle_t t) {
  (void)t;
  pairing_finish(true);
}

static void copy_json_string(const cJSON *item, char *dst, size_t dst_len) {
  if (cJSON_IsString(item) && item->valuestring != NULL && dst_len > 0) {
    strncpy(dst, item->valuestring, dst_len - 1);
    dst[dst_len - 1] = '\0';
  }
}

void prj_confs_receive_data_cb(uint8_t *data, int len) {
  if (data == NULL || len <= 0) {
    return;
  }

  char *json_str = malloc((size_t)len + 1);
  if (json_str == NULL) {
    return;
  }
  memcpy(json_str, data, (size_t)len);
  json_str[len] = '\0';

  cJSON *root = cJSON_Parse(json_str);
  free(json_str);
  if (root == NULL) {
    ESP_LOGW(TAG, "JSON invalido nos dados custom BluFi");
    return;
  }

  prj_data_t *d = prj_data_load();

  copy_json_string(cJSON_GetObjectItem(root, "ssid"), d->sta.ssid,
                   sizeof(d->sta.ssid));
  copy_json_string(cJSON_GetObjectItem(root, "password"), d->sta.password,
                   sizeof(d->sta.password));
  copy_json_string(cJSON_GetObjectItem(root, "client_id"), d->client.client_id,
                   sizeof(d->client.client_id));
  copy_json_string(cJSON_GetObjectItem(root, "mqtt_username"), d->mqtt.username,
                   sizeof(d->mqtt.username));
  copy_json_string(cJSON_GetObjectItem(root, "mqtt_password"), d->mqtt.password,
                   sizeof(d->mqtt.password));

  const cJSON *auth_type = cJSON_GetObjectItem(root, "auth_type");
  if (cJSON_IsNumber(auth_type)) {
    d->autentication.auth_type = (prj_auth_type_t)auth_type->valueint;
  }

  const cJSON *eap_type = cJSON_GetObjectItem(root, "eap_type");
  if (cJSON_IsNumber(eap_type)) {
    d->autentication.eap_type = (prj_eap_type_t)eap_type->valueint;
  }

  const cJSON *eap_method = cJSON_GetObjectItem(root, "eap_method");
  if (cJSON_IsNumber(eap_method)) {
    d->autentication.eap_method = (prj_eap_method_t)eap_method->valueint;
  }

  const cJSON *eap_ttls = cJSON_GetObjectItem(root, "eap_ttls_phase2_method");
  if (cJSON_IsNumber(eap_ttls)) {
    d->autentication.eap_ttls_phase2_method = eap_ttls->valueint;
  }

  copy_json_string(cJSON_GetObjectItem(root, "eap_id"), d->autentication.eap_id,
                   sizeof(d->autentication.eap_id));
  copy_json_string(cJSON_GetObjectItem(root, "eap_username"),
                   d->autentication.eap_username,
                   sizeof(d->autentication.eap_username));
  copy_json_string(cJSON_GetObjectItem(root, "eap_password"),
                   d->autentication.eap_password,
                   sizeof(d->autentication.eap_password));
  copy_json_string(cJSON_GetObjectItem(root, "eap_ca"), d->autentication.eap_ca,
                   sizeof(d->autentication.eap_ca));

  if (d->sta.ssid[0] == '\0') {
    ESP_LOGW(TAG, "JSON sem ssid — ignorado");
    cJSON_Delete(root);
    return;
  }

  d->device.is_to_pair = false;
  d->device.is_configured = true;
  pairing_save_nvs();
  cJSON_Delete(root);

  ESP_LOGI(TAG, "Pareamento OK — SSID='%s' — reinicio para aplicar Wi-Fi",
           d->sta.ssid);

  s_exit = PRJ_CONFS_EXIT_CREDENTIALS;
  if (s_boot_window_active) {
    if (s_pair_timer != NULL) {
      xTimerStop(s_pair_timer, 0);
    }
    pairing_finish(false);
  } else {
    esp_restart();
  }
}

prj_confs_exit_t prj_confs_boot_window(void) {
  if (!prj_data_load()->device.is_to_pair) {
    return PRJ_CONFS_EXIT_FAIL;
  }

  s_boot_window_active = true;
  s_exit = PRJ_CONFS_EXIT_FAIL;
  prj_state_set_state(prj_state_device_pairing, true);

  ESP_LOGI(TAG, "Janela inicial %d ms — BLE '%s' (BluFi permanece ativo)",
           PRJ_PAIR_TIMEOUT, wifi_blufi_gap_name());

  s_pair_timer = xTimerCreate("prj_pair", pdMS_TO_TICKS(PRJ_PAIR_TIMEOUT),
                              pdFALSE, NULL, pair_timer_cb);
  if (s_pair_timer != NULL) {
    xTimerStart(s_pair_timer, 0);
  }

  prj_global_wait_take();
  return s_exit;
}
