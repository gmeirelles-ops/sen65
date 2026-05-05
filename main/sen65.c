#include "sen65.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Protótipos das funções que estão em outro arquivo (port)
// Isso remove o erro de "implicit declaration"
extern esp_err_t sen65_port_init(void);
extern esp_err_t sen65_port_write(uint16_t cmd);
extern esp_err_t sen65_port_read(uint8_t *buf, size_t len);

static const char *TAG = "SEN65_DRIVER";

#define CMD_START 0x0021
#define CMD_READ  0x0446

esp_err_t sen65_init(void)
{
    return sen65_port_init();
}

esp_err_t sen65_start(void)
{
    // Usando o TAG aqui para remover o aviso de "unused variable"
    ESP_LOGI(TAG, "Enviando comando START");
    return sen65_port_write(CMD_START);
}

esp_err_t sen65_read(sen65_data_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;

    // Conforme a tabela: 8 valores * (2 bytes + 1 CRC) = 24 bytes
    uint8_t raw[24]; 

    if (sen65_port_write(CMD_READ) != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao enviar comando de leitura");
        return ESP_FAIL;
    }

    vTaskDelay(pdMS_TO_TICKS(25));

    if (sen65_port_read(raw, sizeof(raw)) != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao ler dados do barramento");
        return ESP_FAIL;
    }

    // Mapeamento pulando os bytes de CRC (índices 2, 5, 8, 11, 14, 17, 20, 23)
    out->pm1_0       = (uint16_t)((raw[0] << 8) | raw[1]);
    out->pm2_5       = (uint16_t)((raw[3] << 8) | raw[4]);
    out->pm4_0       = (uint16_t)((raw[6] << 8) | raw[7]);
    out->pm10        = (uint16_t)((raw[9] << 8) | raw[10]);
    out->humidity    = (int16_t)((raw[12] << 8) | raw[13]);
    out->temperature = (int16_t)((raw[15] << 8) | raw[16]);
    out->voc_index   = (int16_t)((raw[18] << 8) | raw[19]);
    out->nox_index   = (int16_t)((raw[21] << 8) | raw[22]);

    return ESP_OK;
}