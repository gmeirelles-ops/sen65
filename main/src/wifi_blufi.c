#include "wifi_blufi.h"

#include <stdlib.h>
#include <string.h>

#include "blufi_rules.h"
#include "esp_blufi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#if CONFIG_BT_CONTROLLER_ENABLED || !CONFIG_BT_NIMBLE_ENABLED
#include "esp_bt.h"
#endif

static const char *TAG = "wifi_blufi";

static bool s_wifi_stack_up;
static bool s_blufi_host_up;
static bool s_wifi_events_registered;

static void blufi_wifi_event_handler(void *arg, esp_event_base_t event_base,
                                     int32_t event_id, void *event_data) {
  (void)arg;
  (void)event_base;
  (void)event_data;

  if (event_id != WIFI_EVENT_SCAN_DONE) {
    return;
  }

  uint16_t ap_count = 0;
  esp_wifi_scan_get_ap_num(&ap_count);
  if (ap_count == 0) {
    ESP_LOGI(TAG, "Scan Wi-Fi: nenhum AP");
    return;
  }

  wifi_ap_record_t *ap_list =
      (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * ap_count);
  if (ap_list == NULL) {
    esp_wifi_clear_ap_list();
    return;
  }

  if (esp_wifi_scan_get_ap_records(&ap_count, ap_list) != ESP_OK) {
    free(ap_list);
    esp_wifi_clear_ap_list();
    return;
  }

  esp_blufi_ap_record_t *blufi_list =
      (esp_blufi_ap_record_t *)malloc(ap_count * sizeof(esp_blufi_ap_record_t));
  if (blufi_list == NULL) {
    free(ap_list);
    return;
  }

  for (int i = 0; i < ap_count; i++) {
    blufi_list[i].rssi = ap_list[i].rssi;
    memcpy(blufi_list[i].ssid, ap_list[i].ssid, sizeof(ap_list[i].ssid));
  }

  if (blufi_rules_ble_connected()) {
    esp_blufi_send_wifi_list(ap_count, blufi_list);
    ESP_LOGI(TAG, "Scan Wi-Fi: %u redes enviadas ao telefone", ap_count);
  }

  esp_wifi_scan_stop();
  free(ap_list);
  free(blufi_list);
}

esp_err_t wifi_blufi_register_wifi_events(void) {
  if (s_wifi_events_registered) {
    return ESP_OK;
  }
  esp_err_t err = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_SCAN_DONE,
                                             &blufi_wifi_event_handler, NULL);
  if (err == ESP_OK) {
    s_wifi_events_registered = true;
  }
  return err;
}

esp_err_t wifi_blufi_ensure_wifi_stack(void) {
  if (s_wifi_stack_up) {
    return ESP_OK;
  }

  if (esp_netif_get_handle_from_ifkey("WIFI_STA_DEF") == NULL) {
    esp_netif_create_default_wifi_sta();
  }

  esp_err_t err = esp_wifi_init(&(wifi_init_config_t)WIFI_INIT_CONFIG_DEFAULT());
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    return err;
  }

  err = esp_wifi_set_mode(WIFI_MODE_STA);
  if (err != ESP_OK && err != ESP_ERR_WIFI_NOT_INIT) {
    return err;
  }

  err = esp_wifi_set_ps(WIFI_PS_NONE);
  if (err != ESP_OK && err != ESP_ERR_WIFI_NOT_INIT) {
    return err;
  }

  err = esp_wifi_start();
  if (err != ESP_OK && err != ESP_ERR_WIFI_NOT_INIT) {
    return err;
  }

  s_wifi_stack_up = true;
  return ESP_OK;
}

void wifi_blufi_advertise(void) {
  esp_blufi_adv_start();
  ESP_LOGI(TAG, "BLE a anunciar como '%s' (app EspBlufi)", BLUFI_DEVICE_NAME);
}

const char *wifi_blufi_gap_name(void) { return BLUFI_DEVICE_NAME; }

esp_err_t wifi_blufi_start_ble(void) {
  esp_err_t ret;

  if (s_blufi_host_up) {
    wifi_blufi_advertise();
    return ESP_OK;
  }

  ESP_LOGI(TAG, "Iniciando BluFi BLE — nome: %s", BLUFI_DEVICE_NAME);

#if CONFIG_BT_CONTROLLER_ENABLED || !CONFIG_BT_NIMBLE_ENABLED
  if (!esp_bt_controller_get_status()) {
    ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
      ESP_LOGE(TAG, "mem_release: %s", esp_err_to_name(ret));
      return ret;
    }

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
      ESP_LOGE(TAG, "bt_controller_init: %s", esp_err_to_name(ret));
      return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
      ESP_LOGE(TAG, "bt_controller_enable: %s", esp_err_to_name(ret));
      return ret;
    }
  }
#endif

  ret = esp_blufi_host_and_cb_init(blufi_rules_get_callbacks());
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "BluFi host: %s", esp_err_to_name(ret));
    return ret;
  }

  s_blufi_host_up = true;
  ESP_LOGI(TAG, "BluFi versao %04x — aguardando INIT_FINISH para anunciar",
           esp_blufi_get_version());
  return ESP_OK;
}

esp_err_t wifi_blufi_start(void) {
  esp_err_t ret = wifi_blufi_ensure_wifi_stack();
  if (ret != ESP_OK) {
    return ret;
  }
  return wifi_blufi_start_ble();
}

esp_err_t wifi_blufi_stop(void) {
  if (!s_blufi_host_up) {
    return ESP_OK;
  }
  esp_blufi_adv_stop();
  esp_err_t ret = esp_blufi_host_deinit();
  s_blufi_host_up = false;
  s_wifi_stack_up = false;
  return ret;
}
