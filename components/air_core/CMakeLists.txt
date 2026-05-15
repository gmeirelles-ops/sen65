idf_component_register(
    SRCS "src/filter.c"
          "src/baseline.c"
          "src/adaptive_stats.c"
          "src/air_quality_level.c"
          "src/air_aqi.c"
          "src/air_json.c"
          "src/event.c"
          "src/classifier.c"
          "src/telemetry.c"
          "src/series_record.c"
    INCLUDE_DIRS "inc"
    REQUIRES esp_timer nvs_flash freertos log
)
