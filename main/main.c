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
	// communications_common_config();

	/*
	wifi_start();
	vTaskDelay(10000 / portTICK_PERIOD_MS);
	mqtt_start_example();
	*/
	
	adc_configure();

	// Allocating samples buffer
	int *samples_buf = malloc(sizeof(unsigned int) * ADC_SAMPLES_BUFFER_SIZE);

	// Priming buffer
	// adc_priming_samples_buf(samples_buf);

	size_t start = xTaskGetTickCount();
	adc_sampling_once(samples_buf, (float) 100, false);
	size_t end = xTaskGetTickCount();

	size_t ticks = end - start;
	unsigned ms = ticks * portTICK_PERIOD_MS;

	printf("Max sampling frequency (?) declared: %u kHz\n", SOC_ADC_SAMPLE_FREQ_THRES_HIGH / 1000);
	printf("Filling a %d samples buffer took: %u ms\n", ADC_SAMPLES_BUFFER_SIZE, ms);

	/*
	float Ts = (float)ms / ADC_SAMPLES_BUFFER_SIZE;

	float sampling_freq = 1 / Ts; // kHz
	sampling_freq *= 1000; //Hz
	
	printf("Approximate actual sampling frequency: %f Hz\n", sampling_freq);

	int min_val;
	int max_val;
	get_min_max(samples_buf, &min_val, &max_val);
	printf("Min V: %u, Max V: %u, Delta: %u\n", min_val, max_val, max_val - min_val); 

	// TODO: This is where i should adjust the sampling frequency

	get_max_freq(samples_buf, sampling_freq);
	
	*/
	
}
