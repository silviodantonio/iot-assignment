#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h" // I hope that this works
#include "freertos/task.h"
#include "freertos/message_buffer.h"
#include "esp_err.h"
#include "esp_log.h"
// #include "esp_task_wdt.h"
#include "esp_adc/adc_oneshot.h"

#include "mqtt_client.h"
#include "project_constants.h"

#include "sampling.h"
#include "wifi.h"
#include "mqtt.h"

static const char* MAIN_TAG = "Main task";
static adc_oneshot_unit_handle_t adc_handle;
static MessageBufferHandle_t samples_buf_handle;
static unsigned int running_avg_time_window_s;
static float sampling_freq;
static unsigned int wait_time_ticks;
static esp_mqtt_client_handle_t mqtt_client_handle;

void runningAvgTask (void *arg)
{

	unsigned int window_size = sampling_freq * running_avg_time_window_s;
	
	// Wait for a window-time to be sure to have values in the message buf
	vTaskDelay(pdMS_TO_TICKS(running_avg_time_window_s * 1000));

	while(1) {

		int avg = 0;
		int val_read;
		unsigned int samples_count = 0;
		
		while(samples_count < window_size) {
			xMessageBufferReceive(samples_buf_handle, &val_read, sizeof(int), wait_time_ticks);
			avg += (val_read) / window_size;
			samples_count++;
			// esp_task_wdt_reset();
		}

		printf("Average value over %u s: %d\n", running_avg_time_window_s, avg);
		
		// send data via MQTT
		char int_string[11];
		sprintf(int_string, "%d", avg);

		esp_mqtt_client_publish(mqtt_client_handle, "/topic/qos0", int_string, 0, 0, 0);
	}

}

void samplingTask (void *arg)
{
	// Compute delay for obtaining the desired sampling frequency
	unsigned int delay_period_ms = (unsigned int)(1000 / sampling_freq);

	// Sampling loop
	int sample_val;
	while(1) {
		ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &sample_val));
		xMessageBufferSend(samples_buf_handle, &sample_val, sizeof(int), wait_time_ticks);
		vTaskDelay(pdMS_TO_TICKS(delay_period_ms));
	}
	
}

void adjust_sampling_freq()
{
	// Measuring max sampling freq
	int dummy_sample;
	unsigned int num_test_samples = 100000;

	int *samples_buf = calloc(ADC_SAMPLES_BUFFER_SIZE, sizeof(int));

	ESP_LOGI(MAIN_TAG, "Computing max sampling frequency...");

	unsigned int start_time = xTaskGetTickCount();
	for(size_t i = 0; i < num_test_samples; i++){
		adc_oneshot_read(adc_handle, ADC_CHANNEL, &dummy_sample);
		samples_buf[i % ADC_SAMPLES_BUFFER_SIZE] = dummy_sample;
	}
	unsigned int sampling_duration_ticks = xTaskGetTickCount() - start_time;

	float sampling_duration_sec = pdTICKS_TO_MS(sampling_duration_ticks) / 1000;
	float max_sampling_freq_hz = num_test_samples / sampling_duration_sec;
	
	ESP_LOGI(MAIN_TAG, "Took %f seconds to sample %d", sampling_duration_sec, num_test_samples);
	ESP_LOGI(MAIN_TAG, "Estimated max sampling frequency is: %f Hz", max_sampling_freq_hz);

	// printf("Printing samples:");
	// for(int i = 0; i < ADC_SAMPLES_BUFFER_SIZE; i++){
	// 	printf("%d\n", samples_buf[i]);
	// }

	// getting max frequency of sampled signal
	float max_signal_freq = get_max_freq(samples_buf, max_sampling_freq_hz);

	ESP_LOGI(MAIN_TAG, "Frequency with max magnitude: %f Hz", max_signal_freq);

	// adjusting sampling frequency
	if ( max_signal_freq * 2.2 < max_sampling_freq_hz / 2) {
		sampling_freq = max_signal_freq * 2.2;
	}
	else {
		sampling_freq = max_signal_freq * 2;
	}

	ESP_LOGI(MAIN_TAG, "Adjusted sampling freq at %f Hz", sampling_freq);

	free(samples_buf);

}


void app_main(void)
{

	// Configuring and initializing ADC
	adc_oneshot_unit_init_cfg_t adc_unit_cfg = {
		.unit_id = ADC_UNIT,
		.clk_src = ADC_CLK_SRC,
		.ulp_mode = ADC_ULP_MODE_DISABLE,
	};

	ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_unit_cfg, &adc_handle));

	adc_oneshot_chan_cfg_t adc_chan_cfg = {
		.bitwidth = ADC_BITWIDTH_DEFAULT,	// Defaults to max bitwidth (12 bits for ESP32-S3)
		.atten = ADC_ATTEN_DB_12,
	};

	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &adc_chan_cfg));

	adjust_sampling_freq();

	samples_buf_handle = xMessageBufferCreate((sizeof(size_t) + sizeof(int)) * ADC_SAMPLES_BUFFER_SIZE);
	
	// wifi_init();
	// mqtt_client_handle = mqtt_app_start();
	
	wait_time_ticks = pdMS_TO_TICKS(0.8 * (1000 / sampling_freq)); // 80% of sampling period [ms]
	
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

	/*
	running_avg_time_window_s = 1;

	TaskHandle_t runningAvgTask_handle;
	xTaskCreate(
		runningAvgTask,
		"runningAvgTask",
		4096,
		NULL,
		0,
		&runningAvgTask_handle
	);

	*/
	
}
