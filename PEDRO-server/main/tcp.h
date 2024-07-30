#pragma once

esp_err_t create_socket(int* socket_id, int domain, int type, int protocol);
void config_socket(struct sockaddr_in* socket_addr, int domain, int port, unsigned long addr);
esp_err_t bind_socket(int socket_id, const struct sockaddr* socket_addr);
esp_err_t listen_socket(int socket_id);
void tcp_server_task();