idf_component_register(
    SRCS
        "src/main.c"
        "src/i2c_board.c"
        "src/mqtt_air.c"
        "src/prj_network.c"
        "src/prj_data.c"
        "src/prj_global.c"
        "src/prj_state.c"
        "src/prj_confs.c"
        "src/prj_wifi.c"
        "src/prj_mqtt.c"
        "src/blufi_rules.c"
        "src/wifi_blufi.c"
        "src/blufi_init.c"
        "src/blufi_security.c"
    INCLUDE_DIRS "inc"
    REQUIRES air_core sen65 acd1200 driver freertos esp_timer nvs_flash mqtt json
             esp_wifi esp_netif esp_event esp_coex bt mbedtls wpa_supplicant
)
