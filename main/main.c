#include <stdbool.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"

#include "esp_pm.h"
#include "project_constants.h"
#include "common_configs.h"
#include "soc/soc_caps.h"

#include "sampling.h"
// #include "wifi.h"
// #include "mqtt.h"


void app_main(void)
{
	communications_common_config();

	/*
	wifi_start();
	vTaskDelay(10000 / portTICK_PERIOD_MS);
	mqtt_start_example();
	*/
	
	adc_configure();

	// Allocating samples buffer
	int *samples_buf = malloc(sizeof(int) * ADC_SAMPLE_BUFFER_SIZE);

	size_t start = xTaskGetTickCount();
	adc_priming_samples_buf(samples_buf);
	size_t end = xTaskGetTickCount();

	size_t ticks = end - start;
	unsigned ms = ticks * portTICK_PERIOD_MS;

	printf("Max sampling frequency (?) declared: %u kHz\n", SOC_ADC_SAMPLE_FREQ_THRES_HIGH / 1000);
	printf("Filling a 2048 samples buffer took: %u ms\n", ms);
	printf("Approximate actual sampling frequency: %u kHz\n", ADC_SAMPLE_BUFFER_SIZE / ms);

	// TODO: This is where i should adjust the sampling frequency
	
	adc_sampling_start(samples_buf, false);
	
}
