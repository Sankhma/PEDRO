#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_event.h"

#include "wifi.h"
#include "tcp.h"

#define STACK_SIZE 8192

static const char* TAG = "MAIN";

void app_main(void) {

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_LOGD(TAG, "nvs_flash_init succesfull.");

    wifi_start();
    xTaskCreatePinnedToCore(tcp_server_task, "TCP_SERVER", STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL, tskNO_AFFINITY);
}