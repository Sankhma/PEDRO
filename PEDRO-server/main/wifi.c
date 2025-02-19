#include <string.h>

#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_log.h"

static const char* WIFI_TAG = "WIFI_DRIVER";
static const char* WIFI_EVENT_TAG = "WIFI_EVENT_HANDLER";
static const char* IP_EVENT_TAG = "IP_EVENT_HANDLER";

// Move these into the nvs?
#define DEFAULT_RETRY_DELAY 10
#define MAX_RETRY_COUNT 100

#define MAX_SSID_LEN 32
#define MAX_PASSWORD_LEN 64

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

esp_err_t access_point_init() {

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

esp_err_t access_point_config(wifi_ap_config_t* access_point_config) {
    esp_err_t err;

    err = esp_wifi_set_config(WIFI_IF_AP, (wifi_config_t *)access_point_config);
    if(err != ESP_OK) {
        ESP_LOGW(WIFI_TAG, "Failed to configure the wifi driver.");
        return err;
    }
    ESP_LOGD(WIFI_TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d", access_point_config->ssid,
             access_point_config->password, access_point_config->channel);

    return ESP_OK;
}

void access_point_start(const char* wifi_ssid, const char* wifi_password, const int wifi_channel, 
                        const wifi_auth_mode_t wifi_authmode, const int ssid_hidden, const int max_connection,
                        const int beacon_interval) {
    esp_err_t err;

    err = access_point_init();
    ESP_ERROR_CHECK(err);

    wifi_ap_config_t ap_config = {};
    memcpy(&ap_config.ssid, wifi_ssid, (strlen(wifi_ssid) < MAX_SSID_LEN) ? strlen(wifi_ssid) : MAX_SSID_LEN);
    memcpy(&ap_config.password, wifi_password, 
            (strlen(wifi_password) < MAX_PASSWORD_LEN) ? strlen(wifi_password) : MAX_PASSWORD_LEN);
    ap_config.ssid_len = strlen(wifi_ssid);
    ap_config.channel = wifi_channel;
    ap_config.authmode = wifi_authmode;
    ap_config.ssid_hidden = ssid_hidden;
    ap_config.max_connection = max_connection;
    ap_config.beacon_interval = beacon_interval;

    err = access_point_config(&ap_config);
    ESP_ERROR_CHECK(err);

    err = esp_wifi_start();
    ESP_ERROR_CHECK(err);
}

void access_point_stop();