#ifndef SEN65_H
#define SEN65_H

#include <stdint.h>
#include "esp_err.h"

/**
 * @brief Estrutura de dados do SEN65
 * 
 * NOTA: Todos os valores utilizam ponto fixo para evitar processamento float desnecessário.
 * - PM (Particulate Matter): valor real = dado / 10.0 (µg/m³)
 * - Umidade: valor real = dado / 100.0 (%RH)
 * - Temperatura: valor real = dado / 200.0 (°C)
 * - VOC/NOx Index: valor real = dado / 10.0
 */
typedef struct {
    uint16_t pm1_0;        // Escala x10
    uint16_t pm2_5;        // Escala x10
    uint16_t pm4_0;        // Escala x10
    uint16_t pm10;         // Escala x10
    int16_t humidity;      // Escala x100
    int16_t temperature;    // Escala x200
    int16_t voc_index;     // Escala x10
    int16_t nox_index;     // Escala x10
} sen65_data_t;

/**
 * @brief Inicializa o barramento I2C e configura o dispositivo.
 * @return ESP_OK em caso de sucesso.
 */
esp_err_t sen65_init(void);

/**
 * @brief Envia o comando para o sensor iniciar as medições contínuas.
 * @return ESP_OK em caso de sucesso.
 */
esp_err_t sen65_start(void);

/**
 * @brief Lê os dados atuais do sensor (comando 0x0446, 16 bytes MSB primeiro).
 * @param out Ponteiro para a estrutura onde os dados serão salvos.
 * @return ESP_OK em caso de sucesso na transação I2C e parse.
 */
esp_err_t sen65_read(sen65_data_t *out);

#endif // SEN65_H