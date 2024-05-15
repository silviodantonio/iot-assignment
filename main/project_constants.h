#ifndef H_PROJECT_CONSTANTS
#define H_PROJECT_CONSTANTS

// Note: not all constants used in the project are defined here, some of them
// have to be changed directly in the source file.

#define ADC_SAMPLE_BUFFER_SIZE 2048
// Probably would be better if this value is calculated at runtime
#define WINDOWED_AVG_SAMPLES_NUM 5
#define ADC_UNIT ADC_UNIT_1
#define ADC_CHANNEL ADC_CHANNEL_6
#define ADC_CLK_SRC ADC_DIGI_CLK_SRC_PLL_F240M

#define WIFI_SSID "iPhone di Silvio"
#define WIFI_PASS ""

#endif
