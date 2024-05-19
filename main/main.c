#include <stdbool.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h" // I hope that this works
#include "freertos/task.h"
#include "freertos/message_buffer.h"
#include "esp_err.h"
// #include "esp_task_wdt.h"
#include "esp_adc/adc_oneshot.h"

#include "project_constants.h"

#include "sampling.h"
#include "wifi.h"
#include "mqtt.h"

// Creating message buffer for samples
static MessageBufferHandle_t samples_buf_handle;
static unsigned int running_avg_period_s;
static float sampling_freq = 0;
static unsigned int wait_time_ticks;

void runningAvgTask (void *arg)
{

	unsigned int window_size = sampling_freq * running_avg_period_s;

	while(1) {

		int avg = 0;
		int val_read;
		unsigned int samples_count = 0;

		// Compute average only after enough samples have
		// been collected
		vTaskDelay(pdMS_TO_TICKS(running_avg_period_s * 1000));
		
		while(samples_count < window_size) {
			xMessageBufferReceive(samples_buf_handle, &val_read, sizeof(int), wait_time_ticks);
			avg += (val_read) / window_size;
			samples_count++;
			// esp_task_wdt_reset();
		}

		printf("Average value over %u s: %d\n", running_avg_period_s, avg);
		// Then the data should be sent to MQTT
	}

}

void samplingTask (void *arg)
{
	// Configuring and initializing ADC
	adc_oneshot_unit_init_cfg_t adc_unit_cfg = {
		.unit_id = ADC_UNIT,
		.clk_src = ADC_CLK_SRC,
		.ulp_mode = ADC_ULP_MODE_DISABLE,
	};

	adc_oneshot_unit_handle_t adc_handle;	// This could be moved into global scope
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_unit_cfg, &adc_handle));

	adc_oneshot_chan_cfg_t adc_chan_cfg = {
		.bitwidth = ADC_BITWIDTH_DEFAULT,	// Defaults to max bitwidth (12 bits for ESP32-S3)
		.atten = ADC_ATTEN_DB_6,
	};

	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &adc_chan_cfg));

	// Compute delay for obtaining the desired sampling frequency
	unsigned delay = 1000 / sampling_freq;

	// Sampling loop
	while(1) {
		int sample_val;
		int send_ret;
		ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &sample_val));
		// Note: ticks to wait could be set to a value less than the used sampling rate
		// (that must be computed beforehand)
		// printf("ADC Read: %d\n", sample_val);
		send_ret = xMessageBufferSend(samples_buf_handle, &sample_val, sizeof(int), wait_time_ticks);
		// printf("Written %d bytes to message buffer\n", send_ret);
		vTaskDelay(pdMS_TO_TICKS(delay));
	}
	
}


void app_main(void)
{

	// here i should initialize the sampling freq value
	
	wifi_init();
	
	samples_buf_handle = xMessageBufferCreate((sizeof(size_t) + sizeof(int)) * ADC_SAMPLES_BUFFER_SIZE);
	sampling_freq = 10; //Hz
	wait_time_ticks = pdMS_TO_TICKS(0.8 * (1000 / sampling_freq)); // 80% of sampling period [ms]
	running_avg_period_s = 1; // average over 5 seconds time window
	
	TaskHandle_t samplingTask_handle;
	xTaskCreate(
		samplingTask,
		"samplingTask",
		4096,
		NULL,
		0,
		&samplingTask_handle
	);
	// esp_task_wdt_add(samplingTask_handle);

	TaskHandle_t runningAvgTask_handle;
	xTaskCreate(
		runningAvgTask,
		"runningAvgTask",
		4096,
		NULL,
		0,
		&runningAvgTask_handle
	);

	
}
