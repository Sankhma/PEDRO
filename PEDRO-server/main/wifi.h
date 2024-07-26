#pragma once

void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
esp_err_t wifi_ap_init();
esp_err_t wifi_ap_config();
void wifi_start();
void wifi_stop();