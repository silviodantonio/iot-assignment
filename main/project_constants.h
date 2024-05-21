#ifndef H_PROJECT_CONSTANTS
#define H_PROJECT_CONSTANTS

// Note: not all constants used in the project are defined here, some of them
// have to be changed directly in the source files.

#define ADC_SAMPLES_BUFFER_SIZE 1024
#define ADC_UNIT ADC_UNIT_1
#define ADC_CHANNEL ADC_CHANNEL_6
#define ADC_CLK_SRC ADC_DIGI_CLK_SRC_PLL_F240M
#define AVG_TIME_WINDOW_S 5

#define WIFI_SSID "iPhone di Silvio"
#define WIFI_PASS "iotproject"
#define WIFI_MAX_ATTEMPTS 5

#define MQTT_BROKER_ADDRESS "mqtt://mqtt.eclipseprojects.io"

#endif
