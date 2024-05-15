#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi_types.h"

#define WIFI_SSID      "iPhone di Silvio"
#define WIFI_PASS      ""

static const char *WIFI_TAG = "wifi task";

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
        if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
            ESP_LOGI(WIFI_TAG, "Succesfully connected to AP");
        }
        else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t* event = event_data;
            ESP_LOGI(WIFI_TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    }
}

void wifi_start(void)
{

    /* This creates a default network interface that binds
     * the station to the TCP/IP stack */
    esp_netif_create_default_wifi_sta();

    /* Configure and initialize the wifi driver task
     * This task will posts wifi events to the default event loop*/
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handler to the default loop
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_t instance_ap_connected;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL,
      &instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &wifi_event_handler, NULL,
        &instance_ap_connected));

    // Configuring the wifi driver
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t wifi_config = {
      .sta =
          {
              .ssid = WIFI_SSID,
              .password = WIFI_PASS,
          },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));


    /* Start wifi driver task, upon startup the driver generates an
    event that will be handeld by the event handler defined at the top.
    For now this event won't be catched and we will assume that the
    task will always start successfully. */
    ESP_LOGI(WIFI_TAG, "Starting wifi driver");
    ESP_ERROR_CHECK(esp_wifi_start());

    // Scanning and trying to connect to the declared AP.
    // If the driver manages to connect to the AP, it will post a
    // "CONNECTED" event and shortly after triggering the DHCP process
    // that is executed by the netif task.
    // Also in this case, we won't check for the "CONNECTED".
    ESP_LOGI(WIFI_TAG, "Trying to connect to AP");
    esp_wifi_connect();

    // After the DHCP process succesfully terminates, netif task triggers
    // an event "GOT_IP". After this i assume i can safely start to use MQTT.

}
