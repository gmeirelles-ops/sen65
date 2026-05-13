idf_component_register(
    SRCS "main.c" "i2c_board.c"
    INCLUDE_DIRS "."
    REQUIRES sen65 acd1200 driver freertos esp_timer nvs_flash
)
