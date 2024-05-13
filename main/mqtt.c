#include "mqtt_client.h"
#include "esp_event.h"
#include "esp_err.h"
#include <stdint.h>

#define MQTT_BROKER_URI "some uri"

void mqtt_event_handler (void *event_handler_args, esp_event_base_t event_base, int32_t event_id, void *event_data){

	/* event handler recieves context data (last parameter) in esp_mqtt_event_t
	 * it contains
	 *- event id
	 *- client handle
	 *- data
	 *- topic
	 *- msg id
	 *   and so on
	 */

	//TODO: implement

}

esp_err_t mqtt_client_start()
{
	int ret_value = ESP_OK;

	const esp_mqtt_client_config_t mqtt_cfg = {
		.uri = MQTT_BROKER_URI,
	};

	esp_mqtt_client_handle_t mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

	// Probably i should move this somewhere else
	ESP_ERROR_CHECK(esp_event_loop_create_default());

        /* Parameters
         * 1. mqtt client handle
         * 2. (event_id_t) event type
         * 3. event handler
         * 4. event handler arguments
         *
         * subscribing mqtt_event_handler to any event published to the
         * default event loop.
         */
        ret_value = esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
	if (ret_value != ESP_OK) {
		return ret_value;
	}

	ret_value = esp_mqtt_client_start(mqtt_client);
	if (ret_value != ESP_OK) {
		return ret_value;
	}
	
	return ret_value;

}
