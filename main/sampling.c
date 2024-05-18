#include <stdint.h>
#include <stdio.h>
#include "sampling.h"

#include <stdio.h>
#include "esp_log.h"
#include "hal/adc_types.h"
#include "project_constants.h"
#include "esp_adc/adc_oneshot.h"
#include "sdkconfig.h"
#include "soc/clk_tree_defs.h" // For selecting clock sources
#include "esp_err.h"
#include "dsps_fft2r.h"
#include "dsps_tone_gen.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Probably i will move this in a separate .h file

static adc_oneshot_unit_handle_t adc_handle;

static const char *SAMPLING_TAG = "Sampling module";

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
		.atten = ADC_ATTEN_DB_6,
	};

	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &adc_chan_cfg));
}

void adc_priming_samples_buf(int *samples_buf)
{

	for(int i = 0; i < ADC_SAMPLES_BUFFER_SIZE; i++) {
		ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, samples_buf + i));
	}

}

void adc_sampling_once(int *samples_buf, float sampling_freq, bool log)
{
	if(sampling_freq > 1000) {
		ESP_LOGW(SAMPLING_TAG, "Max frequency exceeded");
	}

	// This is too coarse grained: 500Hz sampling frequency requires
	// a t_ms of 2. A sampling frequency of 1000Hz requires a t_ms of
	// 1. All sampling frequency between them are lost since i'm using
	// an int value.
	int t_ms = 1000 / sampling_freq; // sampling frequency in Hz

	ESP_LOGI(SAMPLING_TAG, "Set T = %d ms", t_ms);

	static unsigned int offset = 0;
	while(offset < ADC_SAMPLES_BUFFER_SIZE) {
		ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, samples_buf + offset));
		if(log) {
			printf("ADC: Read %u\n", *(samples_buf + offset));
		}
		offset++;
		vTaskDelay(t_ms / portTICK_PERIOD_MS);
	}
}

void adc_sampling_loop(int *samples_buf, float sampling_freq, bool log)
{
	if(sampling_freq > 1000) {
		ESP_LOGW(SAMPLING_TAG, "Max frequency exceeded");
	}

	int t_ms = 1000 / sampling_freq;

	ESP_LOGI(SAMPLING_TAG, "Set T = %d ms", t_ms);

	// Continuously reading samples
	static unsigned int offset = 0;
	while(1) {
		ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, samples_buf + offset));
		if(log) {
			printf("ADC: Read %u\n", *(samples_buf + offset));
		}
		offset++;
		offset %= ADC_SAMPLES_BUFFER_SIZE;
		vTaskDelay(t_ms / portTICK_PERIOD_MS);
	}
}

void get_min_max(int *samples_buf, int *min_val, int *max_val)
{
	*(min_val) = samples_buf[0];
	*(max_val) = 0;

	for (int i = 0; i < ADC_SAMPLES_BUFFER_SIZE; i++) {
		int val = samples_buf[i];
		if (val > *(max_val))
			*(max_val) = val;
		if (val < *(min_val))
			*(min_val) = val;
	}

}

// This function should use the FFT in order to compute the max frequency of the
// signal that can be used for adjusting the sampling rate of the ADC
void get_max_freq(int *adc_buf, float sampling_freq)
{

	// Allocate FFT working buffer (initialize to all zeroes)
	float *fft_buf = calloc(ADC_SAMPLES_BUFFER_SIZE*2, sizeof(float));
	
	// Move samples into FFT buffer
	for(int i = 0; i < ADC_SAMPLES_BUFFER_SIZE; i++) {
		fft_buf[i * 2] = adc_buf[i];
	}

	// Execute fft
	dsps_fft2r_fc32_ansi_(fft_buf, 0, NULL);

	dsps_bit_rev_fc32_ansi(fft_buf, 0); // Is this for ABS number?
	dsps_cplx2reC_fc32_ansi(fft_buf, 0);  // This de-interleaves, however i think there's also a different function

	// find max value. Since the values should be simmetric, just searching
	// in half of the array.
	int max_index = 0;
	float max_val = fft_buf[0];

	for(int i = 1; i < ADC_SAMPLES_BUFFER_SIZE/2; i++) {
		if (max_val < fft_buf[i]){
			max_index = i;
			max_val = fft_buf[i];
		}
	}

	int min_index = 0;
	float min_val = fft_buf[0];
	for(int i = 1; i < ADC_SAMPLES_BUFFER_SIZE/2; i++) {
		if (min_index > fft_buf[i]){
			min_index = i;
			min_val = fft_buf[i];
		}
	}
	
	// compute frequency of said max value
	float freq_step = sampling_freq / ADC_SAMPLES_BUFFER_SIZE;
	
	printf("Frequency bin size: %f\n", freq_step);

	printf("Max magnitude: %f @ %f\n", max_val, max_index * freq_step);
	printf("Min magnitude: %f @ %f\n", min_val, min_index * freq_step);

	free(fft_buf);
}
