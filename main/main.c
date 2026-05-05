// main/main.c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "sen65.h"

// Definindo a TAG para os logs
static const char *TAG = "SEN65_APP";
static QueueHandle_t sen65_queue;

void sen65_task(void *pvParameters)
{
    sen65_data_t data;

    ESP_LOGI(TAG, "Inicializando sensor...");
    if (sen65_init() != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao inicializar porta I2C!");
        vTaskDelete(NULL);
        return;
    }

    if (sen65_start() != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao enviar comando START!");
    }

    vTaskDelay(pdMS_TO_TICKS(1000));

    while (1) {
        if (sen65_read(&data) == ESP_OK) {
            // Enviando a estrutura completa para a fila
            xQueueSend(sen65_queue, &data, pdMS_TO_TICKS(100));
        } else {
            ESP_LOGW(TAG, "Erro na leitura do sensor.");
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void processing_task(void *pvParameters)
{
    sen65_data_t data;
    while (1) {
        if (xQueueReceive(sen65_queue, &data, portMAX_DELAY)) {
            
            
            // Para PM2.5 (x10), Temp (x200), Hum (x100)
            ESP_LOGI(TAG, "--- NOVA LEITURA ---");
            ESP_LOGI(TAG, "PM2.5: %u.%u ug/m3", data.pm2_5 / 10, data.pm2_5 % 10);
            ESP_LOGI(TAG, "Temp:  %d.%02d C", data.temperature / 200, (data.temperature % 200) / 2);
            ESP_LOGI(TAG, "Hum:   %d.%02d %%", data.humidity / 100, data.humidity % 100);
            ESP_LOGI(TAG, "VOC:   %d.%d", data.voc_index / 10, data.voc_index % 10);
            ESP_LOGI(TAG, "--------------------");
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Iniciando Aplicativo Principal");

    sen65_queue = xQueueCreate(5, sizeof(sen65_data_t));

    xTaskCreate(sen65_task, "sen65_task", 4096, NULL, 5, NULL);
    xTaskCreate(processing_task, "processing_task", 4096, NULL, 4, NULL);
}