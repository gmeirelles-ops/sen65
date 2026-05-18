#include "prj_data.h"

#include <string.h>
#include "esp_log.h"
#include "nvs.h"
#include "prj_defines.h"

static const char *TAG = "prj_data";
static prj_data_t s_data;
static bool s_loaded;

static void prj_data_defaults(prj_data_t *d) {
  memset(d, 0, sizeof(*d));
  d->autentication.auth_type = auth_type_personal;
  d->device.prj_board_type = prj_board_type_sensor;
  d->device.is_configured = false;
  d->device.is_to_pair = true;
  snprintf(d->client.client_id, sizeof d->client.client_id, "sen65");
}

void prj_data_init(void) {
  prj_data_defaults(&s_data);

  nvs_handle_t h;
  esp_err_t err = nvs_open(PRJ_NVS_NAMESPACE, NVS_READONLY, &h);
  if (err != ESP_OK) {
    s_loaded = true;
    return;
  }

  size_t sz = sizeof(s_data);
  if (nvs_get_blob(h, PRJ_NVS_KEY_CFG, &s_data, &sz) != ESP_OK ||
      sz != sizeof(s_data)) {
    prj_data_defaults(&s_data);
  }
  nvs_close(h);
  s_loaded = true;

  ESP_LOGI(TAG, "init: SSID='%s' client='%s' configured=%d to_pair=%d",
           s_data.sta.ssid, s_data.client.client_id,
           (int)s_data.device.is_configured, (int)s_data.device.is_to_pair);
}

prj_data_t *prj_data_load(void) {
  if (!s_loaded) {
    prj_data_init();
  }
  return &s_data;
}

esp_err_t prj_data_save(void) {
  nvs_handle_t h;
  esp_err_t err = nvs_open(PRJ_NVS_NAMESPACE, NVS_READWRITE, &h);
  if (err != ESP_OK) {
    return err;
  }
  err = nvs_set_blob(h, PRJ_NVS_KEY_CFG, &s_data, sizeof(s_data));
  if (err == ESP_OK) {
    err = nvs_commit(h);
  }
  nvs_close(h);
  return err;
}

void prj_data_set_sta(const char *ssid, const char *password) {
  prj_data_t *d = prj_data_load();
  if (ssid != NULL) {
    strncpy(d->sta.ssid, ssid, sizeof(d->sta.ssid) - 1);
    d->sta.ssid[sizeof(d->sta.ssid) - 1] = '\0';
  }
  if (password != NULL) {
    strncpy(d->sta.password, password, sizeof(d->sta.password) - 1);
    d->sta.password[sizeof(d->sta.password) - 1] = '\0';
  }
}

bool prj_data_has_wifi(void) {
  return prj_data_load()->sta.ssid[0] != '\0';
}
