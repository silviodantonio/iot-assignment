#include <stdio.h>
#include "esp_adc/adc_oneshot.h"
#include "hal/adc_types.h"
#include "soc/clk_tree_defs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"

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
	
	// Taking samples
	int read_val;
	while(1) {
		ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL_6, &read_val));
		printf("%d\n", read_val);
		vTaskDelay(100 / portTICK_PERIOD_MS);		// This determines the sampling rate of the ADC
	}

}
