/**
 * @file    prj_wifi.c
 * @brief   Wi-Fi STA a partir de prj_data_t (NVS).
 */

#include "prj_wifi.h"

#include <assert.h>
#include <string.h>

#include "esp_eap_client.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "main.h"
#include "prj_data.h"
#include "prj_defines.h"
#include "prj_global.h"
#include "blufi_rules.h"
#include "prj_state.h"

static const char *TAG = "prj_wifi";
static const int CONNECTED_BIT = BIT0;

static bool s_initialized;
static EventGroupHandle_t s_wifi_event_group;
static prj_data_t *s_data;
static prj_global_instance_t *s_global;
static esp_netif_t *s_sta_netif;
static bool s_connected_on_startup;
static bool s_sta_busy;
static bool s_allow_reconnect = true;

static const char *wifi_disc_reason_str(uint8_t reason) {
  switch (reason) {
    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
      return "4way_timeout (senha errada ou BLE interferiu)";
    case WIFI_REASON_CONNECTION_FAIL:
      return "connection_fail (coexistencia BLE/Wi-Fi)";
    case WIFI_REASON_NO_AP_FOUND:
      return "AP nao encontrado";
    case WIFI_REASON_AUTH_FAIL:
      return "falha de autenticacao";
    default:
      return "ver WIFI_REASON_*";
  }
}

bool prj_wifi_sta_busy(void) { return s_sta_busy; }

static void wifi_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
                         void *event_data);
static void conn_task(void *parm);

static void wifi_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
                         void *event_data) {
  (void)arg;
  (void)event_data;

  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    if (strlen((const char *)s_data->sta.ssid) > 0) {
      esp_wifi_connect();
    } else {
      s_global->volume = PRJ_BOARD_OFFLINE_VOLUME;
      prj_global_wait_give();
    }
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    const wifi_event_sta_disconnected_t *disc = event_data;
    if (disc != NULL) {
      ESP_LOGW(TAG, "desconectado, reason=%u (%s)", disc->reason,
               wifi_disc_reason_str(disc->reason));
    }
    xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
    prj_state_set_state(prj_state_wifi_connected, false);
    if (s_allow_reconnect && !blufi_rules_ble_connected()) {
      esp_wifi_connect();
    } else {
      ESP_LOGI(TAG, "reconnect adiado (BLE ativo ou Wi-Fi em pausa)");
    }
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    s_allow_reconnect = true;
    prj_state_set_state(prj_state_wifi_connected, true);

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(s_sta_netif, &ip_info);
    esp_ip4addr_ntoa(&ip_info.ip, s_global->lan_ip, sizeof(s_global->lan_ip));
    xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
    ESP_LOGI(TAG, "IP: %s", s_global->lan_ip);
  }
}

static void conn_task(void *parm) {
  (void)parm;
  EventBits_t bits = xEventGroupWaitBits(
      s_wifi_event_group, CONNECTED_BIT, pdTRUE, pdFALSE,
      pdMS_TO_TICKS(PRJ_WIFI_CONN_TIMEOUT_MS));

  if (bits & CONNECTED_BIT) {
    s_connected_on_startup = true;
    ESP_LOGI(TAG, "conectado ao AP");
  } else {
    ESP_LOGW(TAG, "timeout %d ms sem IP", PRJ_WIFI_CONN_TIMEOUT_MS);
    esp_wifi_disconnect();
    s_allow_reconnect = false;
  }

  s_sta_busy = false;
  prj_global_wait_give();
  vTaskDelete(NULL);
}

esp_err_t prj_wifi_init(void) {
  esp_err_t err;

  ESP_LOGI(TAG, "%s: start", __func__);

  if (s_initialized) {
    return s_connected_on_startup ? ESP_OK : ESP_FAIL;
  }

  s_data = prj_data_load();

  if (strlen((const char *)s_data->sta.ssid) > 0) {
    s_global = prj_global_instance_get_data();
    s_wifi_event_group = xEventGroupCreate();

    s_sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (s_sta_netif == NULL) {
      s_sta_netif = esp_netif_create_default_wifi_sta();
    }
    assert(s_sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
      ESP_LOGE(TAG, "esp_wifi_init: %s", esp_err_to_name(err));
      return err;
    }

    MRM_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    MRM_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                               &wifi_handler, NULL));
    MRM_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                               &wifi_handler, NULL));
    MRM_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    xTaskCreate(conn_task, "prj_wifi_conn", 4096, NULL, 3, NULL);

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));
    wifi_config.sta.bssid_set = false;

    strncpy((char *)wifi_config.sta.ssid, s_data->sta.ssid,
            sizeof(wifi_config.sta.ssid) - 1);
    wifi_config.sta.ssid[sizeof(wifi_config.sta.ssid) - 1] = '\0';

    if (s_data->autentication.auth_type == auth_type_personal) {
      strncpy((char *)wifi_config.sta.password, s_data->sta.password,
              sizeof(wifi_config.sta.password) - 1);
      wifi_config.sta.password[sizeof(wifi_config.sta.password) - 1] = '\0';
    } else if (s_data->autentication.auth_type == auth_type_enterprise) {
      if (s_data->autentication.eap_type == eap_type_wpa3) {
        wifi_config.sta.pmf_cfg.required = true;
        esp_eap_client_set_ca_cert((const unsigned char *)s_data->autentication.eap_ca,
                                   strlen(s_data->autentication.eap_ca));
      }
      esp_eap_client_set_identity((uint8_t *)s_data->autentication.eap_id,
                                strlen(s_data->autentication.eap_id));
      esp_eap_client_set_username((uint8_t *)s_data->autentication.eap_username,
                                  strlen(s_data->autentication.eap_username));
      esp_eap_client_set_password((uint8_t *)s_data->autentication.eap_password,
                                  strlen(s_data->autentication.eap_password));
      if (s_data->autentication.eap_method == eap_method_ttls) {
        esp_eap_client_set_ttls_phase2_method(
            s_data->autentication.eap_ttls_phase2_method + 1);
      }
      ESP_ERROR_CHECK(esp_wifi_sta_enterprise_enable());
    }

    MRM_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    err = esp_wifi_start();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
      ESP_LOGE(TAG, "esp_wifi_start: %s", esp_err_to_name(err));
      return err;
    }

    s_sta_busy = true;
    esp_wifi_connect();

    prj_global_wait_take();
  }

  s_initialized = true;
  ESP_LOGI(TAG, "%s: end", __func__);

  return s_connected_on_startup ? ESP_OK : ESP_FAIL;
}
