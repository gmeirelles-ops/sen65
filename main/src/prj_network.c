#include "prj_network.h"

#include "esp_coexist.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "prj_confs.h"
#include "prj_data.h"
#include "prj_global.h"
#include "prj_mqtt.h"
#include "prj_wifi.h"
#include "wifi_blufi.h"

static const char *TAG = "prj_network";

esp_err_t prj_network_init(void) {
  prj_data_t *data;
  bool has_ssid;

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(prj_global_init());

  prj_data_init();
  data = prj_data_load();
  has_ssid = strlen((const char *)data->sta.ssid) > 0;

  /*
   * Com credenciais na NVS: conecta Wi-Fi antes do BLE para evitar falha de
   * coexistencia (reason 15/205) e scan BluFi durante assoc.
   */
  if (has_ssid && !data->device.is_to_pair) {
    if (wifi_blufi_ensure_wifi_stack() != ESP_OK) {
      ESP_LOGE(TAG, "wifi stack falhou");
    } else {
      esp_coex_preference_set(ESP_COEX_PREFER_WIFI);
      if (prj_wifi_init() == ESP_OK) {
        esp_err_t m = prj_mqtt_init(prj_global_instance_get_data());
        if (m != ESP_OK) {
          ESP_LOGW(TAG, "prj_mqtt_init: %s", esp_err_to_name(m));
        } else {
          ESP_LOGI(TAG, "Wi-Fi + MQTT prontos");
        }
      } else {
        ESP_LOGW(TAG, "prj_wifi_init falhou");
      }
      esp_coex_preference_set(ESP_COEX_PREFER_BALANCE);
    }

    if (wifi_blufi_start_ble() != ESP_OK) {
      ESP_LOGE(TAG, "wifi_blufi_start_ble falhou");
    } else {
      ESP_LOGI(TAG, "BluFi ativo — '%s' (EspBlufi, sem PoP)", wifi_blufi_gap_name());
    }
    return ESP_OK;
  }

  /* Pareamento ou sem SSID: BluFi (Wi-Fi + BLE) desde o inicio. */
  if (wifi_blufi_start() != ESP_OK) {
    ESP_LOGE(TAG, "wifi_blufi_start falhou");
  } else {
    ESP_LOGI(TAG, "BluFi ativo — '%s' (EspBlufi, sem PoP)", wifi_blufi_gap_name());
  }

  if (data->device.is_to_pair) {
    prj_confs_exit_t ex = prj_confs_boot_window();
    data = prj_data_load();
    if (ex == PRJ_CONFS_EXIT_CREDENTIALS) {
      ESP_LOGI(TAG, "Credenciais recebidas — reinicio");
      esp_restart();
    }
  }

  if (has_ssid) {
    esp_coex_preference_set(ESP_COEX_PREFER_WIFI);
    if (prj_wifi_init() == ESP_OK) {
      esp_err_t m = prj_mqtt_init(prj_global_instance_get_data());
      if (m != ESP_OK) {
        ESP_LOGW(TAG, "prj_mqtt_init: %s", esp_err_to_name(m));
      } else {
        ESP_LOGI(TAG, "Wi-Fi + MQTT prontos (BluFi ativo)");
      }
    } else {
      ESP_LOGW(TAG, "prj_wifi_init falhou (BluFi continua ativo)");
    }
    esp_coex_preference_set(ESP_COEX_PREFER_BALANCE);
  } else {
    ESP_LOGW(TAG, "Sem SSID — configure via EspBlufi");
  }

  return ESP_OK;
}
