#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"

#include "common_configs.h"

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
	int *samples_buf = malloc(sizeof(int) * 1024);

	adc_priming_samples_buf(samples_buf);

	// TODO: This is where i should adjust the sampling frequency
	
	adc_sampling_start(samples_buf);
	
}
