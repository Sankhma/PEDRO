#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs_flash.h"

#include "esp_log.h"

#include "esp_netif.h"
#include "esp_wifi.h"

#include "esp_http_server.h" 

#define MIN(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

#define WIFI_SSID "don't connect"

static const char *TAG = "main";

esp_err_t get_handler(httpd_req_t *req) {
    const char resp[] = "URI GET Response";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t post_handler(httpd_req_t *req) {
    char content[100];

    size_t recv_size = MIN(req->content_len, sizeof(content));

    int ret = httpd_req_recv(req, content, recv_size);
    if(ret <= 0 && ret == HTTPD_SOCK_ERR_TIMEOUT) {
        httpd_resp_send_408(req);
    }

    const char resp[] = "URI POST Response";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

void app_main(void) {

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_LOGD(TAG, "nvs_flash_init succesfull!\n");

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_ap();

    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&config));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .password = "",
            .authmode = WIFI_AUTH_OPEN,
            .max_connection = 10
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    esp_wifi_start();

    httpd_config_t http_config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    httpd_uri_t uri_get = {
        .uri = "/uri",
        .method = HTTP_GET,
        .handler = get_handler,
        .user_ctx = NULL
    };

    httpd_uri_t uri_post = {
        .uri = "/uri",
        .method = HTTP_POST,
        .handler = post_handler,
        .user_ctx = NULL
    };

    httpd_start(&server, &http_config);
    httpd_register_uri_handler(server, &uri_get);
    httpd_register_uri_handler(server, &uri_post);

}
