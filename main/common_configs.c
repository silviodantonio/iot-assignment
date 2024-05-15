#include "common_configs.h"

#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"

void communications_common_config(void)
{
	// Initialize NVS (Non Volatile Storage, i.e.: onboard flash memory)
	// Needed for storing wifi and mqtt config values into flash memory
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
	ESP_ERROR_CHECK(nvs_flash_erase());
	ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	// Creating netif task that automatically manages some wifi events
	// For example it manages DHCP
	ESP_ERROR_CHECK(esp_netif_init());
	
	// Event loop to which Wifi and MQTT post events
	ESP_ERROR_CHECK(esp_event_loop_create_default());

}
