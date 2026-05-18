/**
 ******************************************************************************
 * @file    prj_mqtt.c
 * @author  Eng. Eletricista Andre L. A. Lopes
 * @version V1.0.0
 * @date    Quarta, 11 de maio de 2022
 * @brief   Arquivo fonte do protocolo mqtt.
 ******************************************************************************
 * @attention
 *
 * 					e-mail: andrelopes.al@gmail.com.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "prj_data.h"
#include "prj_global.h"
#include "prj_mqtt.h"
#include "main.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "audio_mem.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include <sys/param.h>
#include "esp_mac.h"
#include "cJSON.h"
#include "prj_state.h"

/* Private types -------------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static const char            *_TAG = "prj_mqtt";
static bool                   _is_it_initialized = false;
static QueueHandle_t          _receive_queue;
static SemaphoreHandle_t      _send_semph;
static prj_data_t            *_prj_data;
static prj_global_instance_t *_global_instance;
static prj_mqtt_data_t       *_prj_mqtt_data;
static uint8_t                _subscribe_all_step = 0;

/* Private functions prototypes ----------------------------------------------*/
static void _prj_mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

/* Public functions ----------------------------------------------------------*/

esp_err_t prj_mqtt_init(prj_global_instance_t *prj_global_instance)
{
	if (_is_it_initialized == false)
	{
		uint8_t MAC[6];
		cJSON *json;
		static char last_will_msg[255];

		_prj_data = prj_data_load();
		_global_instance = prj_global_instance;

		_prj_mqtt_data = audio_calloc(1, sizeof(prj_mqtt_data_t));
		AUDIO_MEM_CHECK(_TAG, _prj_mqtt_data, return ESP_FAIL);

		memset(MAC, 0, sizeof(MAC));
		memset(prj_global_instance->mqtt_device_id, 0, sizeof(prj_global_instance->mqtt_device_id));
		memset(_global_instance->mqtt_ack_topic, 0, sizeof(_global_instance->mqtt_ack_topic));
		memset(prj_global_instance->mqtt_device_topic_confs, 0, sizeof(prj_global_instance->mqtt_device_topic_confs));
		memset(prj_global_instance->mqtt_device_topic_status, 0, sizeof(prj_global_instance->mqtt_device_topic_status));
		memset(prj_global_instance->mqtt_device_topic_sdcard, 0, sizeof(prj_global_instance->mqtt_device_topic_sdcard));
		memset(prj_global_instance->mqtt_device_topic_reset, 0, sizeof(prj_global_instance->mqtt_device_topic_reset));
		memset(prj_global_instance->mqtt_device_topic_run, 0, sizeof(prj_global_instance->mqtt_device_topic_run));
		memset(prj_global_instance->mqtt_live_topic, 0, sizeof(prj_global_instance->mqtt_live_topic));
		memset(prj_global_instance->mqtt_radio_topic, 0, sizeof(prj_global_instance->mqtt_radio_topic));
		memset(prj_global_instance->mqtt_playlist_topic, 0, sizeof(prj_global_instance->mqtt_playlist_topic));
		memset(prj_global_instance->mqtt_sensor_topic, 0, sizeof(prj_global_instance->mqtt_sensor_topic));
		memset(prj_global_instance->mqtt_timers_topic, 0, sizeof(prj_global_instance->mqtt_timers_topic));
		memset(prj_global_instance->mqtt_sntp_topic, 0, sizeof(prj_global_instance->mqtt_sntp_topic));
		memset(prj_global_instance->mqtt_downloads_topic, 0, sizeof(prj_global_instance->mqtt_downloads_topic));
		memset(prj_global_instance->mqtt_reset_topic, 0, sizeof(prj_global_instance->mqtt_reset_topic));
		memset(prj_global_instance->mqtt_confs_topic, 0, sizeof(prj_global_instance->mqtt_confs_topic));
		memset(prj_global_instance->mqtt_inputs_topic, 0, sizeof(prj_global_instance->mqtt_inputs_topic));
		memset(prj_global_instance->mqtt_device_lora_topic, 0, sizeof(prj_global_instance->mqtt_device_lora_topic));

		MRM_ERROR_CHECK(esp_read_mac(MAC, ESP_MAC_WIFI_STA));

		snprintf((char*) prj_global_instance->mqtt_device_id, sizeof(prj_global_instance->mqtt_device_id), "%02X%02X%02X%02X%02X%02X", MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5]);
		snprintf((char*) _global_instance->mqtt_ack_topic, sizeof(_global_instance->mqtt_ack_topic),
		ACK_TOPIC_STR_FRM, _prj_data->client.client_id, prj_global_instance->mqtt_device_id);
		snprintf((char*) prj_global_instance->mqtt_device_topic_confs, sizeof(prj_global_instance->mqtt_device_topic_confs),
		CONFS_TOPIC_STR_FRM, _prj_data->client.client_id, prj_global_instance->mqtt_device_id);
		snprintf((char*) prj_global_instance->mqtt_device_topic_status, sizeof(prj_global_instance->mqtt_device_topic_status),
		STATUS_TOPIC_STR_FRM, _prj_data->client.client_id, prj_global_instance->mqtt_device_id);
		snprintf((char*) prj_global_instance->mqtt_device_topic_sdcard, sizeof(prj_global_instance->mqtt_device_topic_sdcard),
		SDCARD_TOPIC_STR_FRM, _prj_data->client.client_id, prj_global_instance->mqtt_device_id);
		snprintf((char*) prj_global_instance->mqtt_device_topic_reset, sizeof(prj_global_instance->mqtt_device_topic_reset),
		DEVICE_RESET_TOPIC_STR_FRM, _prj_data->client.client_id, prj_global_instance->mqtt_device_id);
		snprintf((char*) prj_global_instance->mqtt_device_topic_run, sizeof(prj_global_instance->mqtt_device_topic_run),
		DEVICE_RUN_TOPIC_STR_FRM, _prj_data->client.client_id, prj_global_instance->mqtt_device_id);

		snprintf((char*) prj_global_instance->mqtt_live_topic, sizeof(prj_global_instance->mqtt_live_topic),
		LIVE_TOPIC_STR_FRM, _prj_data->client.client_id);
		snprintf((char*) prj_global_instance->mqtt_radio_topic, sizeof(prj_global_instance->mqtt_radio_topic),
		RADIO_TOPIC_STR_FRM, _prj_data->client.client_id);
		snprintf((char*) prj_global_instance->mqtt_playlist_topic, sizeof(prj_global_instance->mqtt_playlist_topic),
		PLAYLIST_TOPIC_STR_FRM, _prj_data->client.client_id);
		snprintf((char*) prj_global_instance->mqtt_sensor_topic, sizeof(prj_global_instance->mqtt_sensor_topic),
		SENSOR_TOPIC_STR_FRM, _prj_data->client.client_id);
		snprintf((char*) prj_global_instance->mqtt_timers_topic, sizeof(prj_global_instance->mqtt_timers_topic),
		TIMERS_TOPIC_STR_FRM, _prj_data->client.client_id);
		snprintf((char*) prj_global_instance->mqtt_sntp_topic, sizeof(prj_global_instance->mqtt_sntp_topic),
		GC_SNTP_TOPIC_STR_FRM);
		snprintf((char*) prj_global_instance->mqtt_downloads_topic, sizeof(prj_global_instance->mqtt_downloads_topic),
		DOWNLOADS_TOPIC_STR_FRM, _prj_data->client.client_id);
		snprintf((char*) prj_global_instance->mqtt_reset_topic, sizeof(prj_global_instance->mqtt_reset_topic),
		GLOBAL_RESET_TOPIC_STR_FRM, _prj_data->client.client_id);
		snprintf((char*) prj_global_instance->mqtt_confs_topic, sizeof(prj_global_instance->mqtt_confs_topic),
		GLOBAL_CONFS_TOPIC_STR_FRM, _prj_data->client.client_id);
		snprintf((char*) prj_global_instance->mqtt_inputs_topic, sizeof(prj_global_instance->mqtt_inputs_topic),
		GLOBAL_INPUTS_TOPIC_STR_FRM, _prj_data->client.client_id);
		snprintf((char*) prj_global_instance->mqtt_device_lora_topic, sizeof(prj_global_instance->mqtt_device_lora_topic),
		LORA_DEVICES_TOPIC_STR_FRM, _prj_data->client.client_id);

		prj_global_instance->mqtt_client_config.broker.address.uri = MRM_MQTT_URI;
		prj_global_instance->mqtt_client_config.broker.address.port = MRM_MQTT_PORT;
		prj_global_instance->mqtt_client_config.credentials.username = (const char *) _prj_data->mqtt.username;
		prj_global_instance->mqtt_client_config.credentials.authentication.password = (const char *) _prj_data->mqtt.password;
		ESP_LOGI(_TAG, "Broker: %s | user: %s | client_id: %s",
		         MRM_MQTT_URI,
		         _prj_data->mqtt.username[0] ? _prj_data->mqtt.username : "(vazio)",
		         _prj_data->client.client_id);
		prj_global_instance->mqtt_client_config.session.keepalive = MRM_MQTT_KEEPALIVE;
		prj_global_instance->mqtt_client_config.session.last_will.topic = (const char *) prj_global_instance->mqtt_device_topic_status;
		memset(last_will_msg, 0, sizeof(last_will_msg));
		json = cJSON_CreateObject();
		cJSON_AddStringToObject(json, JSON_FIELD_STATUS, JSON_FIELD_OFFLINE);	
		cJSON_AddStringToObject(json, JSON_FIELD_FW_VERSION, PRJ_VERSIONS);
		char *json_string = cJSON_Print(json);
		snprintf(last_will_msg, sizeof(last_will_msg), "%s", json_string);
		cJSON_Delete(json);
		cJSON_free(json_string);
		prj_global_instance->mqtt_client_config.session.last_will.msg = last_will_msg;
		prj_global_instance->mqtt_client_config.session.last_will.qos = MRM_MQTT_QOS;
		prj_global_instance->mqtt_client_config.session.last_will.retain = true;
		prj_global_instance->mqtt_client_config.session.last_will.msg_len = strlen(last_will_msg);
		prj_global_instance->mqtt_client_config.task.stack_size = MQTT_TASK_STACK_SIZE;
		prj_global_instance->mqtt_client_config.buffer.size = MQTT_PAYLOAD_BUFFER_SIZE;

		prj_global_instance->mqtt_client_handle = esp_mqtt_client_init(&prj_global_instance->mqtt_client_config);

		if (prj_global_instance->mqtt_client_handle == NULL)
		{
			ESP_LOGE(_TAG, "esp_mqtt_client_init error");
			return ESP_FAIL;
		}

		/* The last argument may be used to pass data to the event handler, in this project mqtt_event_handler */
		MRM_ERROR_CHECK(esp_mqtt_client_register_event(prj_global_instance->mqtt_client_handle, ESP_EVENT_ANY_ID, _prj_mqtt_event_handler, NULL));
		MRM_ERROR_CHECK(esp_mqtt_client_start(prj_global_instance->mqtt_client_handle));

		_receive_queue = xQueueCreate(5, sizeof(prj_mqtt_data_t));
		if (_receive_queue == NULL)
		{
			return ESP_FAIL;
		}

		_send_semph = xSemaphoreCreateBinary();
		if (_send_semph == NULL)
		{
			return ESP_FAIL;
		}
		xSemaphoreGive(_send_semph);

		prj_global_wait_take();

		if (!prj_state_get(prj_state_mqtt_connected)) {
			ESP_LOGW(_TAG, "MQTT nao conectou ao broker");
			return ESP_FAIL;
		}

		_is_it_initialized = true;
	}

	return ESP_OK;
}

esp_err_t prj_mqtt_send(prj_mqtt_data_t *prj_mqtt_data)
{
	if (_is_it_initialized != true || prj_mqtt_data == NULL) {
		return ESP_FAIL;
	}

	const char *topic = (const char *)prj_mqtt_data->topic;
	const char *payload = (const char *)prj_mqtt_data->payload;
	int payload_len = (int)strlen(payload);

	xSemaphoreTake(_send_semph, portMAX_DELAY);

	prj_mqtt_data->msgid = esp_mqtt_client_publish(
	    _global_instance->mqtt_client_handle, topic, payload, payload_len,
	    MRM_MQTT_QOS, prj_mqtt_data->retain);

	ESP_LOGI(_TAG, "pub topic=%s qos=%d retain=%d len=%d msg_id=%d",
	         topic, MRM_MQTT_QOS, prj_mqtt_data->retain, payload_len,
	         prj_mqtt_data->msgid);
	ESP_LOGI(_TAG, "pub payload: %s", payload);

	xSemaphoreTake(_send_semph, 1000 / portTICK_PERIOD_MS);
	xSemaphoreGive(_send_semph);

	if (prj_mqtt_data->msgid < 0) {
		ESP_LOGE(_TAG, "pub falhou (topic=%s)", topic);
		xSemaphoreGive(_send_semph);
		return ESP_FAIL;
	}

	return ESP_OK;
}

esp_err_t prj_mqtt_receive(prj_mqtt_data_t *prj_mqtt_data)
{
	if (_is_it_initialized == true)
	{
		if (xQueueReceive(_receive_queue, prj_mqtt_data, 100 / portTICK_PERIOD_MS) != pdPASS)
		{
			return ESP_FAIL;
		}
	}

	return ESP_OK;
}

esp_err_t prj_mqtt_subscribe_all(void)
{
	switch (_subscribe_all_step)
	{
		case 0:
			esp_mqtt_client_subscribe(_global_instance->mqtt_client_handle, (const char*) _global_instance->mqtt_sntp_topic, MRM_MQTT_QOS);
			_subscribe_all_step++;
			break;
		case 1:
			esp_mqtt_client_subscribe(_global_instance->mqtt_client_handle, (const char*) _global_instance->mqtt_confs_topic, MRM_MQTT_QOS);
			_subscribe_all_step++;
			break;
		case 2:
			esp_mqtt_client_subscribe(_global_instance->mqtt_client_handle, (const char*) _global_instance->mqtt_device_topic_reset, MRM_MQTT_QOS);
			_subscribe_all_step++;
			break;
		case 3:
			esp_mqtt_client_subscribe(_global_instance->mqtt_client_handle, (const char*) _global_instance->mqtt_radio_topic, MRM_MQTT_QOS);
			_subscribe_all_step++;
			break;
		case 4:
			esp_mqtt_client_subscribe(_global_instance->mqtt_client_handle, (const char*) _global_instance->mqtt_downloads_topic, MRM_MQTT_QOS);
			_subscribe_all_step++;
			break;
		case 5:
			esp_mqtt_client_subscribe(_global_instance->mqtt_client_handle, (const char*) _global_instance->mqtt_reset_topic, MRM_MQTT_QOS);
			_subscribe_all_step++;
			break;
		case 6:
			esp_mqtt_client_subscribe(_global_instance->mqtt_client_handle, (const char*) _global_instance->mqtt_inputs_topic, MRM_MQTT_QOS);
			_subscribe_all_step++;
			break;
		case 7:
			esp_mqtt_client_subscribe(_global_instance->mqtt_client_handle, (const char*) _global_instance->mqtt_live_topic, MRM_MQTT_QOS);
			_subscribe_all_step++;
			break;
		case 8:
			esp_mqtt_client_subscribe(_global_instance->mqtt_client_handle, (const char*) _global_instance->mqtt_sensor_topic, MRM_MQTT_QOS);
			_subscribe_all_step++;
			break;
		case 9:
			if ((_prj_data->device.prj_board_type == prj_board_type_speaker) || \
				(_prj_data->device.prj_board_type == prj_board_type_mrm_master))
			{
				esp_mqtt_client_subscribe(_global_instance->mqtt_client_handle, (const char*) _global_instance->mqtt_timers_topic, MRM_MQTT_QOS);
			}
			_subscribe_all_step++;
			break;
		case 10:
			if ((_prj_data->device.prj_board_type == prj_board_type_speaker) || \
				(_prj_data->device.prj_board_type == prj_board_type_mrm_master))
			{
				esp_mqtt_client_subscribe(_global_instance->mqtt_client_handle, (const char*) _global_instance->mqtt_playlist_topic, MRM_MQTT_QOS);
			}
			_subscribe_all_step++;
			break;
		case 11:
			if ((_prj_data->device.prj_board_type == prj_board_type_speaker) || \
				(_prj_data->device.prj_board_type == prj_board_type_mrm_master))
			{
				esp_mqtt_client_subscribe(_global_instance->mqtt_client_handle, (const char*) _global_instance->mqtt_device_lora_topic, MRM_MQTT_QOS);
			}
			_subscribe_all_step++;
			break;
		default:
			break;
	}

	return ESP_OK;
}

void prj_mqtt_publish_confs(void)
{
	cJSON *json;

	memset(_prj_mqtt_data, 0, sizeof(prj_mqtt_data_t));
	json = cJSON_CreateObject();
	_prj_mqtt_data->retain = 1;
	snprintf((char*) _prj_mqtt_data->topic, sizeof(_prj_mqtt_data->topic), "%s", _global_instance->mqtt_device_topic_confs);
	cJSON_AddStringToObject(json, JSON_FIELD_DEVICE_ID, (const char*) _global_instance->mqtt_device_id);
	cJSON_AddNumberToObject(json, JSON_FIELD_GROUP_ID, _global_instance->group_id);
	if (_prj_data->device.prj_board_type == prj_board_type_speaker)
	{
		cJSON_AddStringToObject(json, JSON_FIELD_BOARD_TYPE, JSON_FIELD_BOARD_SPEAKER);
	}
	else if (_prj_data->device.prj_board_type == prj_board_type_mrm_master)
	{
		cJSON_AddStringToObject(json, JSON_FIELD_BOARD_TYPE, JSON_FIELD_BOARD_MASTER);
	}
	else if (_prj_data->device.prj_board_type == prj_board_type_sensor)
	{
		cJSON_AddStringToObject(json, JSON_FIELD_BOARD_TYPE, "sensor");
	}
	else
	{
		cJSON_AddStringToObject(json, JSON_FIELD_BOARD_TYPE, JSON_FIELD_BOARD_SLAVE);
	}
	cJSON_AddStringToObject(json, JSON_FIELD_TIMEZONE, SNTP_TZ_DEFAULT);
	cJSON_AddNumberToObject(json, JSON_FIELD_VOLUME, _global_instance->volume);
	cJSON_AddNumberToObject(json, JSON_FIELD_NOISE_GAIN, _global_instance->noise_gain);
	cJSON_AddStringToObject(json, JSON_FIELD_DEVICE_OWNER, (const char *)_prj_data->client.client_id);
	char *json_string = cJSON_Print(json);
	snprintf((char*) _prj_mqtt_data->payload, sizeof(_prj_mqtt_data->payload), "%s", json_string);
	(void)prj_mqtt_send(_prj_mqtt_data);
	cJSON_Delete(json);
	cJSON_free(json_string);
}

void prj_mqtt_publish_sensor(const char *json_payload)
{
	if (!_is_it_initialized || json_payload == NULL) {
		return;
	}
	memset(_prj_mqtt_data, 0, sizeof(prj_mqtt_data_t));
	_prj_mqtt_data->retain = MQTT_RETAIN_AIR;
	snprintf((char *)_prj_mqtt_data->topic, sizeof(_prj_mqtt_data->topic), "%s",
	         _global_instance->mqtt_sensor_topic);
	snprintf((char *)_prj_mqtt_data->payload, sizeof(_prj_mqtt_data->payload),
	         "%s", json_payload);
	(void)prj_mqtt_send(_prj_mqtt_data);
}

void prj_mqtt_publish_pair(void)
{
	cJSON *json;

	memset(_prj_mqtt_data, 0, sizeof(prj_mqtt_data_t));
	json = cJSON_CreateObject();
	_prj_mqtt_data->retain = 0;
	snprintf((char*) _prj_mqtt_data->topic, sizeof(_prj_mqtt_data->topic), "%s/PAIRING", _prj_data->client.client_id);
	cJSON_AddStringToObject(json, JSON_FIELD_PRODUCT_ID, (const char*) DEVICE_PRODUCT_ID);
	cJSON_AddStringToObject(json, JSON_FIELD_DEVICE_ID, (const char*) _global_instance->mqtt_device_id);
	char *json_string = cJSON_Print(json);
	snprintf((char*) _prj_mqtt_data->payload, sizeof(_prj_mqtt_data->payload), "%s", json_string);
	(void)prj_mqtt_send(_prj_mqtt_data);
	cJSON_Delete(json);
	cJSON_free(json_string);
}

/* Private functions ---------------------------------------------------------*/

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void _prj_mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
	ESP_LOGD(_TAG, "MQTT event id=%ld", (long)event_id);

	esp_mqtt_event_handle_t event = event_data;
	esp_mqtt_client_handle_t client = event->client;

	switch ((esp_mqtt_event_id_t) event_id)
	{
		case MQTT_EVENT_CONNECTED:
			ESP_LOGI(_TAG, "MQTT_EVENT_CONNECTED");
			_subscribe_all_step = 0;
			if (_prj_data->device.is_configured == true)
			{
				esp_mqtt_client_subscribe(client, (const char*) _global_instance->mqtt_device_topic_confs, MRM_MQTT_QOS);
			}
			prj_state_set_state(prj_state_mqtt_connected, true);
			prj_global_wait_give();
			break;

		case MQTT_EVENT_DISCONNECTED:
			ESP_LOGI(_TAG, "MQTT_EVENT_DISCONNECTED");
			prj_state_set_state(prj_state_mqtt_connected, false);
			break;

		case MQTT_EVENT_SUBSCRIBED:
			ESP_LOGI(_TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);

			if (_prj_data->device.is_configured == false)
			{
				prj_mqtt_publish_confs();
				prj_mqtt_publish_pair();

				_prj_data->device.is_configured = true;
				_prj_data->device.is_to_pair = false;
				if (prj_data_save() != ESP_OK) {
					ESP_LOGE(_TAG, "NVS save apos registo MQTT falhou");
				}
				/* Sem esp_restart: evita boot loop se o broker reconecta. */
			}
			break;

		case MQTT_EVENT_UNSUBSCRIBED:
			ESP_LOGI(_TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
			break;

		case MQTT_EVENT_PUBLISHED:
			ESP_LOGI(_TAG, "pub confirmada pelo broker, msg_id=%d", event->msg_id);
			xSemaphoreGive(_send_semph);
			break;

		case MQTT_EVENT_DATA:
			ESP_LOGI(_TAG, "MQTT_EVENT_DATA recebido. Tamanho chunk: %d, Offset: %d, Total: %d", event->data_len, event->current_data_offset, event->total_data_len);

			// 1. É o primeiro pedaço da mensagem? (Offset 0)
			if (event->current_data_offset == 0)
			{
            	memset(_prj_mqtt_data, 0, sizeof(prj_mqtt_data_t));
            	memcpy(_prj_mqtt_data->topic, event->topic, event->topic_len);
        	}

			// 2. Copia o chunk atual para a posição correta no buffer acumulador
           	memcpy(_prj_mqtt_data->payload + event->current_data_offset, event->data, event->data_len);

        	// 3. Verifica se a mensagem acabou de ser completada
        	if (event->current_data_offset + event->data_len == event->total_data_len)
        	{
				prj_global_strrmv((char*) _prj_mqtt_data->payload, '\r', sizeof(_prj_mqtt_data->payload));
				xQueueSend(_receive_queue, (void* )_prj_mqtt_data, 5000 / portTICK_PERIOD_MS);
        	}
			break;

		case MQTT_EVENT_ERROR:
			ESP_LOGW(_TAG, "MQTT_EVENT_ERROR");
			if (event->error_handle != NULL) {
				if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
				{
					ESP_LOGW(_TAG, "Transporte: esp-tls=0x%x errno=%d (%s)",
					         event->error_handle->esp_tls_last_esp_err,
					         event->error_handle->esp_transport_sock_errno,
					         strerror(event->error_handle->esp_transport_sock_errno));
				}
				else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED)
				{
					ESP_LOGW(_TAG, "Broker recusou: 0x%x",
					         event->error_handle->connect_return_code);
				}
			}
			prj_state_set_state(prj_state_mqtt_connected, false);
			/* Nao reiniciar: em pareamento sem Wi-Fi isso causava boot loop. */
			prj_global_wait_give();
			break;

		default:
			ESP_LOGI(_TAG, "Other event id:%d", event->event_id);
			break;
	}
}



