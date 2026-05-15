#include "mqtt_air.h"

#include "app_config.h"
#include "esp_err.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if MQTT_AIR_ENABLE

#include "air_json.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/ip4_addr.h"
#include "mqtt_client.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_ble.h"

static const char *TAG = "MQTT_AIR";

static esp_mqtt_client_handle_t s_mqtt;
static bool s_mqtt_started;

#define PROV_QR_VER "v1"
#define QRCODE_BASE_URL "https://espressif.github.io/esp-jumpstart/qrcode.html"

#define PROV_RESET_POLL_MS 200

#if (BLE_PROV_RESET_GPIO >= 0)
static void prov_reset_monitor_task(void *pv) {
  int gpio_num = (int)(intptr_t)pv;
  if (gpio_num < 0) {
    vTaskDelete(NULL);
    return;
  }

  gpio_config_t io = {
      .pin_bit_mask = (1ULL << (unsigned)gpio_num),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  gpio_config(&io);

  ESP_LOGI(TAG,
           "Reset Wi-Fi por GPIO: pin=%d — premir ~%ds nos primeiros %ds apos "
           "ligar para apagar rede e voltar ao BLE.",
           gpio_num, BLE_PROV_RESET_HOLD_MS / 1000,
           BLE_PROV_RESET_WINDOW_MS / 1000);

  vTaskDelay(pdMS_TO_TICKS(800));

  uint32_t held_ms = 0;
  const int max_iter = BLE_PROV_RESET_WINDOW_MS / PROV_RESET_POLL_MS;
  for (int i = 0; i < max_iter; i++) {
    int level = gpio_get_level(gpio_num);
    bool active = BLE_PROV_RESET_GPIO_ACTIVE_LOW ? (level == 0) : (level != 0);
    if (active) {
      held_ms += (uint32_t)PROV_RESET_POLL_MS;
      if (held_ms >= (uint32_t)BLE_PROV_RESET_HOLD_MS) {
        ESP_LOGW(TAG,
                 "Reset Wi-Fi por GPIO %d — a limpar NVS Wi-Fi e a reiniciar.",
                 gpio_num);
        (void)esp_wifi_restore();
        vTaskDelay(pdMS_TO_TICKS(300));
        esp_restart();
      }
    } else {
      held_ms = 0;
    }
    vTaskDelay(pdMS_TO_TICKS(PROV_RESET_POLL_MS));
  }
  vTaskDelete(NULL);
}
#endif

static void mqtt_on_event(void *handler_args, esp_event_base_t base,
                           int32_t event_id, void *event_data) {
  (void)handler_args;
  (void)base;
  esp_mqtt_event_handle_t ev = (esp_mqtt_event_handle_t)event_data;
  switch ((esp_mqtt_event_id_t)event_id) {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "MQTT broker conectado (%s)", MQTT_BROKER_URI);
    break;
  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGW(TAG, "MQTT desconectado");
    break;
  case MQTT_EVENT_ERROR:
    if (ev != NULL && ev->error_handle != NULL &&
        ev->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
      ESP_LOGW(TAG, "MQTT erro de transporte");
    }
    break;
  default:
    break;
  }
}

static void start_mqtt_once(void) {
  if (s_mqtt_started) {
    return;
  }
  s_mqtt_started = true;

  static char client_id[24];
  uint8_t mac[6];
  if (esp_read_mac(mac, ESP_MAC_WIFI_STA) == ESP_OK) {
    snprintf(client_id, sizeof client_id, "sen65-%02x%02x%02x", mac[3], mac[4],
             mac[5]);
  } else {
    snprintf(client_id, sizeof client_id, "sen65-air");
  }

  esp_mqtt_client_config_t mqtt = {
      .broker.address.uri = MQTT_BROKER_URI,
      .credentials.client_id = client_id,
      .session.keepalive = 60,
  };
  s_mqtt = esp_mqtt_client_init(&mqtt);
  if (s_mqtt == NULL) {
    ESP_LOGE(TAG, "esp_mqtt_client_init falhou");
    s_mqtt_started = false;
    return;
  }
  ESP_ERROR_CHECK(esp_mqtt_client_register_event(
      s_mqtt, ESP_EVENT_ANY_ID, mqtt_on_event, NULL));
  ESP_ERROR_CHECK(esp_mqtt_client_start(s_mqtt));
  ESP_LOGI(TAG, "MQTT -> %s topico %s", MQTT_BROKER_URI, MQTT_TOPIC_AIR);
}

static void on_got_ip(void *event_data) {
  ip_event_got_ip_t *ev = (ip_event_got_ip_t *)event_data;
  ESP_LOGI(TAG, "IP: " IPSTR, IP2STR(&ev->ip_info.ip));
  start_mqtt_once();
}

static void wifi_start_sta(void) {
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());
}

static void get_ble_device_name(char *out, size_t max) {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  snprintf(out, max, "AIR_%02X%02X%02X", mac[3], mac[4], mac[5]);
}

static void ble_prov_log_qr(const char *name, const char *pop) {
  char payload[180];
  snprintf(payload, sizeof payload,
           "{\"ver\":\"%s\",\"name\":\"%s\",\"pop\":\"%s\",\"transport\":\"%s\"}",
           PROV_QR_VER, name, pop, "ble");
  ESP_LOGI(TAG,
           ">>> Provisioning BLE ativo. App: \"ESP BLE Provisioning\" (Espressif) "
           "na Play Store / App Store.");
  ESP_LOGI(TAG, "Nome BLE: %s | PoP (Security 1): %s", name, pop);
  ESP_LOGI(TAG, "Ou abra no PC: %s?data=%s", QRCODE_BASE_URL, payload);
}

static void air_net_event(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
  (void)arg;

  if (event_base == WIFI_PROV_EVENT) {
    switch (event_id) {
    case WIFI_PROV_START:
      ESP_LOGI(TAG, "Provisioning Wi-Fi iniciado (BLE)");
      break;
    case WIFI_PROV_CRED_RECV: {
      wifi_sta_config_t *cfg = (wifi_sta_config_t *)event_data;
      ESP_LOGI(TAG, "Credenciais recebidas via BLE — SSID: %s",
               (const char *)cfg->ssid);
      break;
    }
    case WIFI_PROV_CRED_FAIL: {
      wifi_prov_sta_fail_reason_t *reason =
          (wifi_prov_sta_fail_reason_t *)event_data;
      ESP_LOGE(TAG, "Falha ao ligar ao AP após provisioning: %s",
               (*reason == WIFI_PROV_STA_AUTH_ERROR)
                   ? "erro de autenticação"
                   : "AP não encontrado");
      break;
    }
    case WIFI_PROV_CRED_SUCCESS:
      ESP_LOGI(TAG, "Wi-Fi associado com sucesso após provisioning");
      break;
    case WIFI_PROV_END:
      ESP_LOGI(TAG, "Provisioning finalizado; BLE pode ser libertado.");
      wifi_prov_mgr_deinit();
      break;
    default:
      break;
    }
  } else if (event_base == WIFI_EVENT) {
    if (event_id == WIFI_EVENT_STA_START) {
      esp_wifi_connect();
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
      ESP_LOGW(TAG, "Wi-Fi desconectado; a reconectar…");
      esp_wifi_connect();
    }
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    on_got_ip(event_data);
  } else if (event_base == PROTOCOMM_TRANSPORT_BLE_EVENT) {
    if (event_id == PROTOCOMM_TRANSPORT_BLE_CONNECTED) {
      ESP_LOGI(TAG, "BLE: telemóvel ligado ao provisioning");
    } else if (event_id == PROTOCOMM_TRANSPORT_BLE_DISCONNECTED) {
      ESP_LOGI(TAG, "BLE: telemóvel desligado");
    }
  } else if (event_base == PROTOCOMM_SECURITY_SESSION_EVENT) {
    if (event_id == PROTOCOMM_SECURITY_SESSION_SETUP_OK) {
      ESP_LOGI(TAG, "Sessão segura de provisioning estabelecida");
    } else if (event_id ==
               PROTOCOMM_SECURITY_SESSION_INVALID_SECURITY_PARAMS) {
      ESP_LOGE(TAG, "Parâmetros de segurança inválidos no provisioning");
    } else if (event_id == PROTOCOMM_SECURITY_SESSION_CREDENTIALS_MISMATCH) {
      ESP_LOGE(TAG, "PoP incorreto — use PoP: %s", BLE_PROV_POP);
    }
  }
}

esp_err_t mqtt_air_init(void) {
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID,
                                             &air_net_event, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(
      PROTOCOMM_TRANSPORT_BLE_EVENT, ESP_EVENT_ANY_ID, &air_net_event, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(
      PROTOCOMM_SECURITY_SESSION_EVENT, ESP_EVENT_ANY_ID, &air_net_event,
      NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                             &air_net_event, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                             &air_net_event, NULL));

  esp_netif_create_default_wifi_sta();
  wifi_init_config_t wcfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&wcfg));

#if BLE_PROV_CLEAR_STORED_WIFI
  ESP_LOGW(TAG,
           "BLE_PROV_CLEAR_STORED_WIFI=1: a limpar credenciais Wi-Fi em NVS "
           "(volte a 0 apos gravar e reprovisionar).");
  ESP_ERROR_CHECK(esp_wifi_restore());
#endif

#if (BLE_PROV_RESET_GPIO >= 0)
  BaseType_t ok = xTaskCreate(prov_reset_monitor_task, "prov_rst", 3072,
                              (void *)(intptr_t)BLE_PROV_RESET_GPIO, 5, NULL);
  if (ok != pdPASS) {
    ESP_LOGW(TAG, "Nao foi possivel criar tarefa de reset GPIO Wi-Fi");
  }
#endif

  wifi_prov_mgr_config_t prov = {
      .scheme = wifi_prov_scheme_ble,
      .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM,
  };
  ESP_ERROR_CHECK(wifi_prov_mgr_init(prov));

  bool provisioned = false;
  ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

  {
    wifi_config_t wc = {0};
    if (esp_wifi_get_config(WIFI_IF_STA, &wc) == ESP_OK) {
      size_t ssid_len = strnlen((const char *)wc.sta.ssid, sizeof(wc.sta.ssid));
      ESP_LOGI(TAG,
               "Wi-Fi em NVS: SSID len=%u%s", (unsigned)ssid_len,
               ssid_len ? " (sem BLE ate repor Wi-Fi: GPIO ou apagar NVS)" : "");
    }
  }

  ESP_LOGI(
      TAG, "Provisionamento: %s",
      provisioned
          ? "JA GUARDADO — BLE desligado. Novo cliente: botao GPIO "
            "(BLE_PROV_RESET_GPIO) ou apagar NVS."
          : "AGUARDANDO — app ESP BLE Provisioning; Android: ativar localizacao.");

  if (!provisioned) {
    char service_name[16];
    get_ble_device_name(service_name, sizeof service_name);

    const char *pop = BLE_PROV_POP;
    ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(
        WIFI_PROV_SECURITY_1, pop, service_name, NULL));
    ble_prov_log_qr(service_name, pop);
  } else {
    ESP_LOGI(TAG, "Wi-Fi já provisionado (NVS); a iniciar STA.");
    wifi_prov_mgr_deinit();
    wifi_start_sta();
  }

  return ESP_OK;
}

void mqtt_air_publish(const air_processed_data_t *processed) {
  if (s_mqtt == NULL || processed == NULL || !s_mqtt_started) {
    return;
  }
  char buf[720];
  int n = air_json_encode_reading(processed, buf, sizeof buf);
  if (n < 0) {
    return;
  }
  int id = esp_mqtt_client_publish(s_mqtt, MQTT_TOPIC_AIR, buf, 0, MQTT_QOS_AIR,
                                   MQTT_RETAIN_AIR);
  if (id < 0) {
    ESP_LOGD(TAG, "publish MQTT indisponível");
  }
}

#else /* !MQTT_AIR_ENABLE */

esp_err_t mqtt_air_init(void) { return ESP_OK; }

void mqtt_air_publish(const air_processed_data_t *processed) {
  (void)processed;
}

#endif
