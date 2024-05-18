#include <stdbool.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h" // I hope that this works
#include "freertos/task.h"
#include "freertos/message_buffer.h"
#include "esp_err.h"
#include "esp_task_wdt.h"
#include "esp_adc/adc_oneshot.h"

#include "esp_pm.h"
#include "project_constants.h"
#include "common_configs.h"
#include "soc/soc_caps.h"

#include "sampling.h"
// #include "wifi.h"
// #include "mqtt.h"

//
// Creating message buffer for samples
static MessageBufferHandle_t samples_buf_handle;

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
	// Configuring and initializing ADC
	adc_oneshot_unit_init_cfg_t adc_unit_cfg = {
		.unit_id = ADC_UNIT,
		.clk_src = ADC_CLK_SRC,	// Uses ABP_CLK, should default at 40MHz
		.ulp_mode = ADC_ULP_MODE_DISABLE,
	};

	// adc unit handle is passed as an argument, will it work?
	adc_oneshot_unit_handle_t adc_handle;
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_unit_cfg, &adc_handle));

	adc_oneshot_chan_cfg_t adc_chan_cfg = {
		.bitwidth = ADC_BITWIDTH_DEFAULT,	// Defaults to max bitwidth (12 bits for ESP32-S3)
		.atten = ADC_ATTEN_DB_6,
	};

	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &adc_chan_cfg));

	// Sampling loop
	while(1) {
		int sample_val;
		int send_ret;
		ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &sample_val));
		// Note: ticks to wait could be set to a value less than the used sampling rate
		// (that must be computed beforehand)
		printf("ADC Read: %d\n", sample_val);
		send_ret = xMessageBufferSend(samples_buf_handle, &sample_val, sizeof(int), 0);
		printf("Written %d bytes to message buffer\n", send_ret);
		vTaskDelay(pdMS_TO_TICKS(10));
	}

	


}


void app_main(void)
{
	// here the tasks for comms should be started (wifi and MQTT)
	
	// Create the task that computes average before starting to sample
	// continuously
	// TaskHandle_t computeAvgTask_handle;
	// xTaskCreate(
	// 	computeAvgTask, // name of func that implements the task
	// 	"computeAvgTask", // descriptive name
	// 	4096, // stack size in words (esp32s3 is a 32 bit architecture)
	// 	(void*)&samples_buf // values passed to the task function
	// 	0, // task priority
	// 	&computeAvgTask_handle // handle for managing the task
	// );
	// esp_task_wdt_add(computeAvgTask_handle);

	
	// Commented for now, it produces a stack overflow somewhere
	samples_buf_handle = xMessageBufferCreate((sizeof(size_t) + sizeof(int)) * ADC_SAMPLES_BUFFER_SIZE);
	
	TaskHandle_t samplingTask_handle;
	xTaskCreate(
		samplingTask, // name of func that implements the task
		"samplingTask", // descriptive name
		4096, // stack size in words (esp32s3 is a 32 bit architecture)
		NULL, // values passed to the task function
		0, // task priority
		&samplingTask_handle // handle for managing the task
	);
	// esp_task_wdt_add(samplingTask_handle);

	
}
