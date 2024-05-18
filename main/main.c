#include <stdbool.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_task_wdt.h"

#include "esp_pm.h"
#include "project_constants.h"
#include "common_configs.h"
#include "soc/soc_caps.h"

#include "sampling.h"
// #include "wifi.h"
// #include "mqtt.h"


void computeAvgTask (void *arg)
{
	int *samples_buf = arg;
	int tail_pos = 0;

	while(1) {
		int avg = window_avg(samples_buf, ADC_SAMPLES_BUFFER_SIZE, tail_pos, WINDOWED_AVG_SAMPLES_NUM);
		
		// This printf will need to be a value posted to the imput queue for
		// the MQTT module
		// printf("AVERAGE: %d\n", avg);
		tail_pos++;
		tail_pos %= ADC_SAMPLES_BUFFER_SIZE;
		esp_task_wdt_reset();
	}

}

void samplingTask (void *arg)
{
	int *samples_buf = arg;

	// Measuring sampling rate (this could be moved in a test task
	size_t start = xTaskGetTickCount();
	adc_priming_samples_buf(samples_buf);
	size_t end = xTaskGetTickCount();

	size_t exec_time_ticks = end - start;
	unsigned exec_time_ms = exec_time_ticks * portTICK_PERIOD_MS;

	printf("Max sampling frequency (?) declared: %u kHz\n", SOC_ADC_SAMPLE_FREQ_THRES_HIGH / 1000);
	printf("Filling a %d samples buffer took: %u ms\n", ADC_SAMPLES_BUFFER_SIZE, exec_time_ms);

	float Ts = (float)exec_time_ms / ADC_SAMPLES_BUFFER_SIZE;

	float sampling_freq = 1 / Ts; // kHz
	sampling_freq *= 1000; //Hz
	
	printf("Approximate actual sampling frequency: %f Hz\n", sampling_freq);

	// i should add inside this esp_task_wdt_reset();
	// however is probably better if i unpack this loop inside
	// the task
	adc_sampling_loop(samples_buf, (float)1000, true);

}

void app_main(void)
{
	// here the tasks for comms should be started (wifi and MQTT)

	// Creating buffer for samples
	int *samples_buf = malloc(sizeof(unsigned int) * ADC_SAMPLES_BUFFER_SIZE);

	//maybe is a good idea to prime the samples buffer
	adc_configure();
	adc_priming_samples_buf(samples_buf);
	
	// Create the task that computes average before starting to sample
	// continuously
	TaskHandle_t computeAvgTask_handle;
	xTaskCreate(
		computeAvgTask, // name of func that implements the task
		"computeAvgTask", // descriptive name
		4096, // stack size in words (esp32s3 is a 32 bit architecture)
		(void*)samples_buf, // values passed to the task function
		0, // task priority
		&computeAvgTask_handle // handle for managing the task
	);
	esp_task_wdt_add(computeAvgTask_handle);

	
	// Commented for now, it produces a stack overflow somewhere
	
	TaskHandle_t samplingTask_handle;
	xTaskCreate(
		samplingTask, // name of func that implements the task
		"samplingTask", // descriptive name
		4096, // stack size in words (esp32s3 is a 32 bit architecture)
		(void*)samples_buf, // values passed to the task function
		1, // task priority
		&samplingTask_handle // handle for managing the task
	);
	esp_task_wdt_add(samplingTask_handle);

	
}
