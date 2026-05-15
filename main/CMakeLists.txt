idf_component_register(
    SRCS "main.c" "i2c_board.c" "mqtt_air.c"
    INCLUDE_DIRS "."
    REQUIRES air_core sen65 acd1200 driver freertos esp_timer nvs_flash mqtt
             esp_wifi esp_netif esp_event wifi_provisioning
)
