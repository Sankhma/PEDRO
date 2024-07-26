#include <string.h>

#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_log.h"

static const char* WIFI_TAG = "WIFI_DRIVER";
static const char* WIFI_EVENT_TAG = "WIFI_EVENT_HANDLER";
static const char* IP_EVENT_TAG = "IP_EVENT_HANDLER";

#define WIFI_SSID "don't connect"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 1
#define WIFI_AUTHMODE WIFI_AUTH_OPEN
#define WIFI_HIDDEN 0
#define WIFI_MAX_CONNECTION ESP_WIFI_MAX_CONN_NUM
#define WIFI_BEACON_INTERVAL 100

#define DEFAULT_RETRY_DELAY 10
#define MAX_RETRY_COUNT 100

static EventGroupHandle_t s_wifi_event_group;

void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    switch(event_id) {
        case WIFI_EVENT_AP_START:
            ESP_LOGD(WIFI_EVENT_TAG, "Access point has been started.");
            break;
        case WIFI_EVENT_AP_STOP:
            ESP_LOGD(WIFI_EVENT_TAG, "Access point has been stopped.");
            break;
        case WIFI_EVENT_AP_STACONNECTED:
            ESP_LOGD(WIFI_EVENT_TAG, "Station connected.");
            break;
        case WIFI_EVENT_AP_STADISCONNECTED:
            ESP_LOGD(WIFI_EVENT_TAG, "Station disconnected.");
            break;
        default:

    }
}

void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    switch(event_id) {
        case IP_EVENT_AP_STAIPASSIGNED:
            ESP_LOGD(IP_EVENT_TAG, "Assigned IP to the station.");
            break;
        default:

    }
}

esp_err_t wifi_ap_init() {

    esp_err_t err;
    int retry_count;

    ESP_ERROR_CHECK(esp_netif_init()); // Initializes the TCP/IP stack
    ESP_LOGD(WIFI_TAG, "Initialized netif, TCP/IP stack.");

    // Creates a default event loop, delays if there is not enough memory and retries
    err = esp_event_loop_create_default();
    retry_count = 0;
    while(err == ESP_ERR_NO_MEM) {
        ESP_LOGE(WIFI_TAG, "Not enough memory to create the default event loop, retrying.");
        vTaskDelay(DEFAULT_RETRY_DELAY);
        err = esp_event_loop_create_default();
        if (++retry_count >= MAX_RETRY_COUNT) {
            ESP_LOGW(WIFI_TAG, "Reached max retry count.");
            return ESP_FAIL;
        }
    }
    if(err == ESP_ERR_INVALID_STATE) {
        ESP_LOGE(WIFI_TAG, "The default event loop has been already created.");
    } else if(err != ESP_OK) {
        ESP_LOGW(WIFI_TAG, "Failed to create the default event loop.");
        return err;
    }
    ESP_LOGD(WIFI_TAG, "Default event loop created succesfully.");

    esp_netif_create_default_wifi_ap(); // Creates the default wifi netif object

    // Initializing wifi with default config, delays if there is not enough memory and retries
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&config);
    retry_count = 0;
    while(err == ESP_ERR_NO_MEM) {  
        ESP_LOGE(WIFI_TAG, "Not enough memory to initialize wifi driver, retrying.");
        vTaskDelay(DEFAULT_RETRY_DELAY);
        err = esp_wifi_init(&config);
        if (++retry_count >= MAX_RETRY_COUNT) {
            ESP_LOGW(WIFI_TAG, "Reached max retry count.");
            return ESP_FAIL;
        }
    }
    if(err != ESP_OK) {
        ESP_LOGW(WIFI_TAG, "Failed to initialize the wifi driver. %d", err);
        return err;
    }
    ESP_LOGD(WIFI_TAG, "Initialized the wifi driver succesfully.");

    // Configures the mode of the Wifi driver
    err = esp_wifi_set_mode(WIFI_MODE_AP);
    if(err == ESP_ERR_WIFI_NOT_INIT) {
        ESP_LOGE(WIFI_TAG, "Failed to set the mode of WIFI, the driver hasn't been initialized.");
        return err;
    } else if(err != ESP_OK) {
        ESP_LOGW(WIFI_TAG, "The mode of the wifi driver couldn't be set.");
        return err;
    }
    ESP_LOGD(WIFI_TAG, "Configured the wifi driver succesfully.");

    s_wifi_event_group = xEventGroupCreate();

    // Registers the handler for event base: WIFI_EVENT with the default event loop
    err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    retry_count = 0;
    while(err == ESP_ERR_NO_MEM) {
        ESP_LOGE(WIFI_TAG, "Not enough memory to register an event handler.");
        vTaskDelay(DEFAULT_RETRY_DELAY);
        err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
        if (++retry_count >= MAX_RETRY_COUNT) {
            ESP_LOGW(WIFI_TAG, "Reached max retry count.");
            return ESP_FAIL;
        }
    }
    if(err != ESP_OK) {
        ESP_LOGW(WIFI_TAG, "Failed to register an event handler.");
    }
    ESP_LOGD(WIFI_TAG, "Registered an event handler for the default event loop for WIFI_EVENT event base.");

    // Registers the handler for event base: IP_EVENT with the default event loop
    err = esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL, NULL);
    retry_count = 0;
    while(err == ESP_ERR_NO_MEM) {
        ESP_LOGE(WIFI_TAG, "Not enough memory to register an event handler.");
        vTaskDelay(DEFAULT_RETRY_DELAY);
        err = esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL, NULL);
        if (++retry_count >= MAX_RETRY_COUNT) {
            ESP_LOGW(WIFI_TAG, "Reached max retry count.");
            return ESP_FAIL;
        }
    }
    if(err != ESP_OK) {
        ESP_LOGW(WIFI_TAG, "Failed to register an event handler.");
    }
    ESP_LOGD(WIFI_TAG, "Registered an event handler for the default event loop for IP_EVENT event base.");

    return ESP_OK;
}

esp_err_t wifi_ap_config() {
    esp_err_t err;

    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .ssid_len = strlen(WIFI_SSID),
            .channel = WIFI_CHANNEL,
            .authmode = WIFI_AUTHMODE,
            .ssid_hidden = WIFI_HIDDEN,
            .max_connection = WIFI_MAX_CONNECTION,
            .beacon_interval = WIFI_BEACON_INTERVAL,
        },
    };
    err = esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config);
    if(err != ESP_OK) {
        ESP_LOGW(WIFI_TAG, "Failed to configure the wifi driver.");
        return err;
    }
    ESP_LOGD(WIFI_TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d", wifi_ap_config.ap.ssid, wifi_ap_config.ap.password, wifi_ap_config.ap.channel);

    return ESP_OK;
}

void wifi_start() {
    ESP_ERROR_CHECK(wifi_ap_init());
    ESP_ERROR_CHECK(wifi_ap_config());
    ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_stop() {} // TODO