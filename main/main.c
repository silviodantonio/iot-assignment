#include <stdio.h>
#include "esp_adc/adc_oneshot.h"
#include "hal/adc_types.h"
#include "soc/clk_tree_defs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"

#define SAMPLES_BUF_LEN 1024
#define WINDOW_AVG_LEN 5

int window_avg(int *array_buf, const unsigned int buf_len, unsigned int tail_pos, const unsigned int window_len) {

	unsigned int start_point = (tail_pos - window_len) % buf_len;
	int avg = 0;
	int samples = 0;
	for(int i = start_point; i != tail_pos; i = (i + 1) % buf_len) {
		avg += *(array_buf + i) / window_len;
		samples++;
	}
	printf("Average on %d samples\n", samples);
	return avg;

}

void app_main(void)
{

	// Configuring and initializing ADC
	adc_oneshot_unit_init_cfg_t adc_unit_cfg = {
		.unit_id = ADC_UNIT_1,
		.clk_src = ADC_DIGI_CLK_SRC_DEFAULT,	// Uses ABP_CLK, should default at 40MHz
		.ulp_mode = ADC_ULP_MODE_DISABLE,
	};

	adc_oneshot_unit_handle_t adc_handle;
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_unit_cfg, &adc_handle));

	adc_oneshot_chan_cfg_t adc_chan_cfg = {
		.bitwidth = ADC_BITWIDTH_DEFAULT,	// Defaults to max bitwidth (12 bits for ESP32-S3)
		.atten = ADC_ATTEN_DB_0,
	};

	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_6, &adc_chan_cfg));
	
	// Priming samples buffer
	int *samples_buf = malloc(sizeof(int) * SAMPLES_BUF_LEN);

	for(int i = 0; i < SAMPLES_BUF_LEN; i++) {
		ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL_6, samples_buf + i));
	}

	// TODO: This is where i should adjust the sampling frequency

	// Getting samples and computing moving average
	unsigned int offset = 0;
	while(1) {
		ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL_6, samples_buf + offset));
		printf("ADC: Read %d\n", *(samples_buf + offset));
		printf("ADC: Running avg: %d\n", window_avg(samples_buf, SAMPLES_BUF_LEN, offset, WINDOW_AVG_LEN));
		offset++;
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}

}
