idf_component_register(SRCS "main.c" "wifi.c" "common_configs.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_adc esp_event esp_wifi nvs_flash mqtt)
