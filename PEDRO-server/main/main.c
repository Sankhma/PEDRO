#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_event.h"
#include "driver/gpio.h"

// Constants
#define STACK_SIZE 8192
#define TAG "MAIN"

#include "wifi.c"
#include "tcp.c"

void app_main(void) {

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_LOGD(TAG, "nvs_flash_init succesfull.");

    // for testing purposes only
    gpio_config_t GPIO_config = {};
    GPIO_config.pin_bit_mask = 1 << 4;
    GPIO_config.mode = GPIO_MODE_INPUT;
    GPIO_config.pull_up_en = GPIO_PULLUP_ENABLE;
    GPIO_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    GPIO_config.intr_type = GPIO_INTR_DISABLE;

    gpio_config(&GPIO_config);
    gpio_dump_io_configuration(stdout, (1 << 4));

    access_point_start("*test*", "", 1, WIFI_AUTH_OPEN, 0, ESP_WIFI_MAX_CONN_NUM, 100);

    xTaskCreatePinnedToCore(tcp_server_task, "TCP_SERVER", STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL, tskNO_AFFINITY);
}