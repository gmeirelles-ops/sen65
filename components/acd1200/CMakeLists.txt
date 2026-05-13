idf_component_register(
    SRCS "src/acd1200.c"
    INCLUDE_DIRS "inc"
    REQUIRES driver freertos esp_timer
)
