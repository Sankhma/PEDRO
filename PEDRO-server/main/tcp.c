#include "esp_log.h"
#include "sys/socket.h"
#include "netinet/in.h"

#define PORT 7777
#define MAX_PENDING_CONNECTIONS 32

static const char* SOCKET_TAG = "SOCKET";

esp_err_t create_socket(int* socket_id, int domain, int type, int protocol) {
    int res = 0;
    res = socket(domain, type, protocol);
    if(res < 0) {
        ESP_LOGE(SOCKET_TAG, "Failed to create a socket.");
        return ESP_FAIL;
    }
    *socket_id = res;
    ESP_LOGD(SOCKET_TAG, "Created a socket succesfully.");
    return ESP_OK;
}

void config_socket(struct sockaddr_in* socket_addr, int domain, int port, unsigned long addr) {
    memset(socket_addr, 0, sizeof(*socket_addr));
    socket_addr->sin_family = domain;
    socket_addr->sin_port = htons(port);
    socket_addr->sin_addr.s_addr = htonl(addr);
}

esp_err_t bind_socket(int socket_id, const struct sockaddr* socket_addr) {
    int err = 0;
    err = bind(socket_id, socket_addr, sizeof(*socket_addr));
    if(err != 0) {
        ESP_LOGE(SOCKET_TAG, "Failed to bind the socket: %d", socket_id);
        close(socket_id);
        return ESP_FAIL;
    }
    ESP_LOGD(SOCKET_TAG, "The socket: %d has been bound succefully.", socket_id);
    return ESP_OK;
}

esp_err_t listen_socket(int socket_id) {
    int err = 0;
    err = listen(socket_id, MAX_PENDING_CONNECTIONS);
    if(err != 0) {
        ESP_LOGE(SOCKET_TAG, "Failed to listen on the socket: %d", socket_id);
        close(socket_id);
        return ESP_FAIL;
    }
    ESP_LOGD(SOCKET_TAG, "The socket: %d has started listening.", socket_id);
    return ESP_OK;
}

void tcp_server_task() {
    int socket_id;
    struct sockaddr sock_addr;

    ESP_ERROR_CHECK(create_socket(&socket_id, AF_INET, SOCK_STREAM, IPPROTO_TCP));
    config_socket((struct sockaddr_in*) &sock_addr, AF_INET, PORT, IPADDR_ANY);
    ESP_ERROR_CHECK(bind_socket(socket_id, &sock_addr));
    ESP_ERROR_CHECK(listen_socket(socket_id));

    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    socklen_t client_addrlen;
    char addr_str[128];
    while(true) {
        int client_id = accept(socket_id, (struct sockaddr *)&client_addr, &client_addrlen);
        if(client_id == -1) {
            ESP_LOGE(SOCKET_TAG, "Failed to accept a connection.");
            close(socket_id);
            // TODO : check for memory leak
            vTaskDelete(NULL);
        }
        inet_ntoa_r(client_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
        ESP_LOGD(SOCKET_TAG, "Accepted a connection from: %s", addr_str);

        // xTaskCreatePinnedToCore(socket_handler, "SOCKET_HANDLER", STACK_SIZE, &client_id, tskIDLE_PRIORITY, NULL, tskNO_AFFINITY);

    }
}