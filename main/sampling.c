#include "sampling.h"

#include <stdio.h>
#include "project_constants.h"
#include "esp_adc/adc_oneshot.h"
#include "soc/clk_tree_defs.h" // For selecting clock sources
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Probably i will move this in a separate .h file

static adc_oneshot_unit_handle_t adc_handle;

int window_avg(int *array_buf, const unsigned int buf_len, unsigned int tail_pos, const unsigned int window_len)
{

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

void adc_configure()
{
	// Configuring and initializing ADC
	adc_oneshot_unit_init_cfg_t adc_unit_cfg = {
		.unit_id = ADC_UNIT,
		.clk_src = ADC_CLK_SRC,	// Uses ABP_CLK, should default at 40MHz
		.ulp_mode = ADC_ULP_MODE_DISABLE,
	};

	// adc unit handle is passed as an argument, will it work?
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_unit_cfg, &adc_handle));

	adc_oneshot_chan_cfg_t adc_chan_cfg = {
		.bitwidth = ADC_BITWIDTH_DEFAULT,	// Defaults to max bitwidth (12 bits for ESP32-S3)
		.atten = ADC_ATTEN_DB_0,
	};

	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &adc_chan_cfg));
}

void adc_priming_samples_buf(int *samples_buf)
{

	for(int i = 0; i < ADC_SAMPLE_BUFFER_SIZE; i++) {
		ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, samples_buf + i));
	}

}

void adc_sampling_start(int *samples_buf, bool log)
{
	// Continuously reading samples
	static unsigned int offset = 0;
	while(1) {
		ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, samples_buf + offset));
		if(!log) {
			printf("ADC: Read %d\n", *(samples_buf + offset));
		}
		offset++;
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}
