#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_netif_types.h"
#include "esp_system.h"
#include "esp_event.h"
#include "project_constants.h"

#include "esp_log.h"
#include "esp_wifi_types.h"
#include "freertos/idf_additions.h"
#include "freertos/event_groups.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "freertos/projdefs.h"
#include "mqtt_client.h"

#define WIFI_GOT_IP BIT0

char *MQTT_TAG = "MQTT";

static const char *BROKER_ADDRESS = MQTT_BROKER_ADDRESS;
static EventGroupHandle_t wifi_event_group;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(MQTT_TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;


    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:

        msg_id = esp_mqtt_client_subscribe(client, "/topic/soda/avg-samples", 0);
        ESP_LOGI(MQTT_TAG, "sent subscribe successful, msg_id=%d", msg_id);

        /*
        // Measuring RTT
        msg_id = esp_mqtt_client_subscribe(client, "/topic/soda/rtt", 0);
        ESP_LOGI(MQTT_TAG, "sent subscribe successful, msg_id=%d", msg_id);

        for (int i = 0; i < 3; i++) {
                char timestamp_str[24];
                sprintf(timestamp_str, "%lu", xTaskGetTickCount());
                esp_mqtt_client_publish(client, "/topic/soda/rtt", timestamp_str, 0, 0, 0);
        };
        */

        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DATA");
        
        /*
        // Uncomment this for measuring packet latencies
        // i couldn't get strcmp to work correctly
        if (strcmp(event->topic, "/topic/soda/rtt") == 0) {

            unsigned int arrived_timestamp = xTaskGetTickCount();
            unsigned int sent_timestamp = atoi(event->data);
            unsigned int latency = (arrived_timestamp - sent_timestamp) / 2;

            ESP_LOGI(MQTT_TAG, "Message had latency: %lu mS\n", pdTICKS_TO_MS(latency));
        }
        */

        

        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);

        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(MQTT_TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(MQTT_TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_init_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(wifi_event_group, WIFI_GOT_IP);
        ESP_LOGI(MQTT_TAG, "Starting MQTT module, event bit set");
    }

}

esp_mqtt_client_handle_t mqtt_app_start(void)
{
    ESP_LOGI(MQTT_TAG, "Starting module");

    wifi_event_group = xEventGroupCreate();

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = BROKER_ADDRESS,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    // esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_measure_rtt, NULL);

    esp_event_handler_instance_t mqtt_init_handler_instance;
    esp_event_handler_instance_register(IP_EVENT,
                               IP_EVENT_STA_GOT_IP, 
                               &mqtt_init_handler,
                               NULL,
                               &mqtt_init_handler_instance);

    //wait for event group
    EventBits_t return_bit = xEventGroupWaitBits(wifi_event_group,
                        WIFI_GOT_IP,
                        pdFALSE,
                        pdFALSE,
                        pdMS_TO_TICKS( WIFI_MAX_ATTEMPTS * 3 * 1000));

    // if everyting is ok then start mqtt
    if ((return_bit & BIT0) == true) {
        esp_mqtt_client_start(client);
        return client;
    }
    else {
        ESP_LOGW(MQTT_TAG, "Couldn't start, didn't get an IP address");
        return NULL;
    }

}
