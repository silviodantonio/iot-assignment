idf_component_register(
                    SRCS "sampling.c" "wifi.c" "mqtt.c" "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_pm esp_adc esp_event esp_wifi nvs_flash mqtt
)
