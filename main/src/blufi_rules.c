/*
 * Callbacks BluFi — Wi-Fi (SSID/senha/lista) + JSON custom (prj_confs).
 */

#include "blufi_rules.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "esp_blufi.h"
#include "esp_blufi_api.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "prj_confs.h"
#include "prj_data.h"
#include "prj_state.h"
#include "prj_wifi.h"
#include "wifi_blufi.h"

static wifi_config_t s_sta_config;
static bool s_ble_connected;

static void blufi_rules_event_callback(esp_blufi_cb_event_t event,
                                       esp_blufi_cb_param_t *param);

static void blufi_save_sta_and_restart(void) {
  prj_data_t *d = prj_data_load();

  prj_data_set_sta((const char *)s_sta_config.sta.ssid,
                   (const char *)s_sta_config.sta.password);
  d->device.is_to_pair = false;
  d->device.is_configured = true;

  if (prj_data_save() != ESP_OK) {
    BLUFI_ERROR("NVS save falhou");
    return;
  }

  BLUFI_INFO("Wi-Fi salvo — SSID='%s' — reinicio", d->sta.ssid);
  esp_restart();
}

static esp_blufi_callbacks_t s_blufi_callbacks = {
    .event_cb = blufi_rules_event_callback,
    .negotiate_data_handler = blufi_dh_negotiate_data_handler,
    .encrypt_func = blufi_aes_encrypt,
    .decrypt_func = blufi_aes_decrypt,
    .checksum_func = blufi_crc_checksum,
};

esp_blufi_callbacks_t *blufi_rules_get_callbacks(void) {
  return &s_blufi_callbacks;
}

bool blufi_rules_ble_connected(void) { return s_ble_connected; }

static void blufi_rules_event_callback(esp_blufi_cb_event_t event,
                                       esp_blufi_cb_param_t *param) {
  switch (event) {
    case ESP_BLUFI_EVENT_INIT_FINISH:
      BLUFI_INFO("BLUFI init finish");
      esp_blufi_adv_start();
      break;
    case ESP_BLUFI_EVENT_DEINIT_FINISH:
      BLUFI_INFO("BLUFI deinit finish");
      break;
    case ESP_BLUFI_EVENT_BLE_CONNECT:
      BLUFI_INFO("BLUFI ble connect");
      s_ble_connected = true;
      esp_blufi_adv_stop();
      blufi_security_init();
      break;
    case ESP_BLUFI_EVENT_BLE_DISCONNECT:
      BLUFI_INFO("BLUFI ble disconnect");
      s_ble_connected = false;
      blufi_security_deinit();
      esp_blufi_adv_start();
      if (!prj_state_get(prj_state_wifi_connected)) {
        esp_wifi_connect();
      }
      break;
    case ESP_BLUFI_EVENT_GET_WIFI_LIST: {
      if (prj_wifi_sta_busy()) {
        BLUFI_INFO("scan ignorado — STA a associar (aguarde ou feche EspBlufi)");
        esp_blufi_send_error_info(ESP_BLUFI_WIFI_SCAN_FAIL);
        break;
      }
      wifi_scan_config_t scan = {
          .ssid = NULL,
          .bssid = NULL,
          .channel = 0,
          .show_hidden = false,
      };
      esp_err_t err = esp_wifi_scan_start(&scan, false);
      if (err != ESP_OK) {
        BLUFI_ERROR("scan_start: %s", esp_err_to_name(err));
        esp_blufi_send_error_info(ESP_BLUFI_WIFI_SCAN_FAIL);
      }
      break;
    }
    case ESP_BLUFI_EVENT_GET_WIFI_STATUS: {
      wifi_mode_t mode;
      esp_blufi_extra_info_t info = {0};

      esp_wifi_get_mode(&mode);
      if (prj_state_get(prj_state_wifi_connected)) {
        esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_SUCCESS, 0,
                                        &info);
      } else {
        esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_FAIL, 0,
                                        &info);
      }
      break;
    }
    case ESP_BLUFI_EVENT_REQ_CONNECT_TO_AP:
      esp_wifi_disconnect();
      esp_wifi_connect();
      break;
    case ESP_BLUFI_EVENT_SET_WIFI_OPMODE:
      esp_wifi_set_mode(param->wifi_mode.op_mode);
      break;
    case ESP_BLUFI_EVENT_REPORT_ERROR:
      BLUFI_ERROR("BLUFI report error, code %d", param->report_error.state);
      esp_blufi_send_error_info(param->report_error.state);
      break;
    case ESP_BLUFI_EVENT_RECV_SLAVE_DISCONNECT_BLE:
      esp_blufi_disconnect();
      break;
    case ESP_BLUFI_EVENT_RECV_STA_SSID:
      memset(s_sta_config.sta.ssid, 0, sizeof(s_sta_config.sta.ssid));
      memcpy(s_sta_config.sta.ssid, param->sta_ssid.ssid, param->sta_ssid.ssid_len);
      s_sta_config.sta.ssid[param->sta_ssid.ssid_len] = '\0';
      esp_wifi_set_config(WIFI_IF_STA, &s_sta_config);
      BLUFI_INFO("Recv STA SSID %s", s_sta_config.sta.ssid);
      break;
    case ESP_BLUFI_EVENT_RECV_STA_PASSWD:
      memset(s_sta_config.sta.password, 0, sizeof(s_sta_config.sta.password));
      memcpy(s_sta_config.sta.password, param->sta_passwd.passwd,
             param->sta_passwd.passwd_len);
      s_sta_config.sta.password[param->sta_passwd.passwd_len] = '\0';
      esp_wifi_set_config(WIFI_IF_STA, &s_sta_config);
      BLUFI_INFO("Recv STA PASSWORD (%u bytes)", param->sta_passwd.passwd_len);
      blufi_save_sta_and_restart();
      break;
    case ESP_BLUFI_EVENT_RECV_CUSTOM_DATA:
      BLUFI_INFO("Recv Custom Data %" PRIu32, param->custom_data.data_len);
      prj_confs_receive_data_cb(param->custom_data.data,
                                (int)param->custom_data.data_len);
      break;
    default:
      break;
  }
}
