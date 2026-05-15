idf_component_register(
    SRCS "src/sen65.c"
          "src/sen65_task.c"
    INCLUDE_DIRS "inc"
    REQUIRES air_core driver acd1200 freertos esp_timer
)
