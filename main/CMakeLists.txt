idf_component_register(
                    SRCS "common_configs.c" "sampling.c" "wifi.c" "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_pm esp_adc esp_event esp_wifi nvs_flash mqtt
)
