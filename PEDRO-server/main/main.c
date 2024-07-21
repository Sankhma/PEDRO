#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs_flash.h"

#include "esp_log.h"

#include "esp_netif.h"
#include "esp_wifi.h"

#include "sys/socket.h"
#include "netinet/in.h"

#include "esp_mac.h"

#include "esp_event.h"

#define DEFAULT_RETRY_DELAY 10
#define MAX_RETRY_COUNT 100

#define WIFI_SSID "don't connect"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 1
#define WIFI_AUTHMODE WIFI_AUTH_OPEN
#define WIFI_HIDDEN 0
#define WIFI_MAX_CONNECTION ESP_WIFI_MAX_CONN_NUM
#define WIFI_BEACON_INTERVAL 100

#define PORT 7777
#define MAX_PENDING_CONNECTIONS 32

static const char *TAG = "MAIN";

static EventGroupHandle_t s_wifi_event_group;

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    switch(event_id) {
        case WIFI_EVENT_AP_START:
            ESP_LOGD("WIFI_EVENT_HANDLER", "Access point has been started.");
            break;
        case WIFI_EVENT_AP_STOP:
            ESP_LOGD("WIFI_EVENT_HANDLER", "Access point has been stopped.");
            break;
        case WIFI_EVENT_AP_STACONNECTED:
            ESP_LOGD("WIFI_EVENT_HANDLER", "Station connected.");
            break;
        case WIFI_EVENT_AP_STADISCONNECTED:
            ESP_LOGD("WIFI_EVENT_HANDLER", "Station disconnected.");
            break;
        default:

    }
}

static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    switch(event_id) {
        case IP_EVENT_AP_STAIPASSIGNED:
            ESP_LOGD("IP_EVENT_HANDLER", "Assigned IP to the station.");
            break;
        default:

    }
}

esp_err_t wifi_ap_init(const char* log_tag) {
    // TODO : Create a function/macro for the ESP_ERR_NO_MEM scenario

    esp_err_t err;
    int retry_count;

    ESP_ERROR_CHECK(esp_netif_init()); // Initializes the TCP/IP stack
    ESP_LOGD(log_tag, "Initialized netif, TCP/IP stack.");

    // Creates a default event loop, delays if there is not enough memory and retries
    err = esp_event_loop_create_default();
    retry_count = 0;
    while(err == ESP_ERR_NO_MEM) {
        ESP_LOGE(log_tag, "Not enough memory to create the default event loop, retrying.");
        vTaskDelay(DEFAULT_RETRY_DELAY);
        err = esp_event_loop_create_default();
        if (++retry_count >= MAX_RETRY_COUNT) {
            ESP_LOGW(log_tag, "Reached max retry count.");
            return ESP_FAIL;
        }
    }
    if(err == ESP_ERR_INVALID_STATE) {
        ESP_LOGE(log_tag, "The default event loop has been already created.");
    } else if(err != ESP_OK) {
        ESP_LOGW(log_tag, "Failed to create the default event loop.");
        return err;
    }
    ESP_LOGD(log_tag, "Default event loop created succesfully.");

    esp_netif_create_default_wifi_ap(); // Creates the default wifi netif object

    // Initializing wifi with default config, delays if there is not enough memory and retries
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&config);
    retry_count = 0;
    while(err == ESP_ERR_NO_MEM) {  
        ESP_LOGE(log_tag, "Not enough memory to initialize wifi driver, retrying.");
        vTaskDelay(DEFAULT_RETRY_DELAY);
        err = esp_wifi_init(&config);
        if (++retry_count >= MAX_RETRY_COUNT) {
            ESP_LOGW(log_tag, "Reached max retry count.");
            return ESP_FAIL;
        }
    }
    if(err != ESP_OK) {
        ESP_LOGW(log_tag, "Failed to initialize the wifi driver. %d", err);
        return err;
    }
    ESP_LOGD(log_tag, "Initialized the wifi driver succesfully.");

    // Configures the mode of the Wifi driver
    err = esp_wifi_set_mode(WIFI_MODE_AP);
    if(err == ESP_ERR_WIFI_NOT_INIT) {
        ESP_LOGE(log_tag, "Failed to set the mode of WIFI, the driver hasn't been initialized.");
        return err;
    } else if(err != ESP_OK) {
        ESP_LOGW(log_tag, "The mode of the wifi driver couldn't be set.");
        return err;
    }
    ESP_LOGD(log_tag, "Configured the wifi driver succesfully.");

    // Registers the handler for event base: WIFI_EVENT with the default event loop
    err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    retry_count = 0;
    while(err == ESP_ERR_NO_MEM) {
        ESP_LOGE(log_tag, "Not enough memory to register an event handler.");
        vTaskDelay(DEFAULT_RETRY_DELAY);
        err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
        if (++retry_count >= MAX_RETRY_COUNT) {
            ESP_LOGW(log_tag, "Reached max retry count.");
            return ESP_FAIL;
        }
    }
    if(err != ESP_OK) {
        ESP_LOGW(log_tag, "Failed to register an event handler.");
    }
    ESP_LOGD(log_tag, "Registered an event handler for the default event loop for WIFI_EVENT event base.");

    // Registers the handler for event base: IP_EVENT with the default event loop
    err = esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL, NULL);
    retry_count = 0;
    while(err == ESP_ERR_NO_MEM) {
        ESP_LOGE(log_tag, "Not enough memory to register an event handler.");
        vTaskDelay(DEFAULT_RETRY_DELAY);
        err = esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL, NULL);
        if (++retry_count >= MAX_RETRY_COUNT) {
            ESP_LOGW(log_tag, "Reached max retry count.");
            return ESP_FAIL;
        }
    }
    if(err != ESP_OK) {
        ESP_LOGW(log_tag, "Failed to register an event handler.");
    }
    ESP_LOGD(log_tag, "Registered an event handler for the default event loop for IP_EVENT event base.");

    return ESP_OK;
}

esp_err_t wifi_ap_config(const char* log_tag) {
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
        ESP_LOGW(log_tag, "Failed to configure the wifi driver.");
        return err;
    }
    ESP_LOGD(log_tag, "wifi_init_softap finished. SSID:%s password:%s channel:%d", wifi_ap_config.ap.ssid, wifi_ap_config.ap.password, wifi_ap_config.ap.channel);

    return ESP_OK;
}

void app_main(void) {

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_LOGD(TAG, "nvs_flash_init succesfull.");

    s_wifi_event_group = xEventGroupCreate();

    // ----------------------------------------------
    // TODO : Move this all to a seperate file
    ESP_ERROR_CHECK(wifi_ap_init("WIFI_INIT"));
    ESP_ERROR_CHECK(wifi_ap_config("WIFI_CONFIG"));
    ESP_ERROR_CHECK(esp_wifi_start());

    // ----------------------------------------------

    int socket_id = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(socket_id < 0) {
        ESP_LOGE(TAG, "Failed to create a socket.");
        abort();
    }
    ESP_LOGD(TAG, "Created a socket succesfully.");

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int err = bind(socket_id, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if(err != 0) {
        ESP_LOGE(TAG, "Failed to bind the socket: %d", socket_id);
        close(socket_id);
        abort();
    }
    ESP_LOGD(TAG, "The socket: %d has been bound succefully.", socket_id);

    err = listen(socket_id, MAX_PENDING_CONNECTIONS);
    if(err != 0) {
        ESP_LOGE(TAG, "Failed to listen on the socket: %d", socket_id);
        close(socket_id);
        abort();
    }
    ESP_LOGD(TAG, "The socket: %d has started listening.", socket_id);

    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    socklen_t client_addrlen;
    char addr_str[128];
    while(true) {
        int client_id = accept(socket_id, (struct sockaddr *)&client_addr, &client_addrlen);
        if(client_id == -1) {
            ESP_LOGE(TAG, "Failed to accept a connection.");
            close(socket_id);
            abort();
        }
        inet_ntoa_r(client_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
        ESP_LOGD(TAG, "Accepted a connection from: %s", addr_str);
    }
}