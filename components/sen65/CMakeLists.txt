idf_component_register(
    SRCS "src/sen65.c"
          "src/sen65_task.c"
          "src/filter.c"
          "src/baseline.c"
          "src/event.c"
          "src/classifier.c"
          "src/telemetry.c"
          "src/series_record.c"
    INCLUDE_DIRS "inc"
    REQUIRES driver acd1200 freertos esp_timer
)
