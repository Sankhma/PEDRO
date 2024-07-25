#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "esp_event.h"

#include "wifi.h"

#define PORT 7777
#define MAX_PENDING_CONNECTIONS 32

static const char *TAG = "MAIN";

void app_main(void) {

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_LOGD(TAG, "nvs_flash_init succesfull.");

    wifi_boot();

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