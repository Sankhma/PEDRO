#include "esp_log.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "driver/gpio.h"

#define PORT 7777
#define MAX_PENDING_CONNECTIONS 32

#define LOCK_CHECK_DELAY 5

#define STACK_SIZE 8192

#define kilobytes(x) {x * 1024}

static const char* SOCKET_TAG = "SOCKET";

const int data_points_num = 5;
const char* data_points_array[] = {
    "GPIO0",
    "GPIO1",
    "GPIO2",
    "GPIO3",
    "GPIO4"
};

typedef enum {
    GPIO0 = 0,
    GPIO1,
    GPIO2,
    GPIO3,
    GPIO4
} DATA_POINTS;

typedef struct message {
    int buffer_size;
    char *buffer;

    int message_type;
};

typedef struct client_data {
    int client_id;

    message recv_message;
    message send_message;
};

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

int receive_message(int client_id, char *buffer, int buffer_size) {
    int bytes_received = 0;
    bytes_received = recv(client_id, buffer, buffer_size, 0);
    if(bytes_received < 0) {
        // TODO: Error Logging
        bytes_received = 0;
    }
    return bytes_received;
}

int send_message(int client_id, char *buffer, int message_size) {
    int bytes_sent = 0;
    bytes_sent = send(client_id, buffer, message_size, 0);
    if(bytes_sent < 0) {
        // TODO: Error Logging
        bytes_sent = 0;
    }
    return bytes_sent;
}

void tcp_data_transfer(void *parameters) {
    while(true) {
        // receive_message();

        // parse_message();

        // prepare_message();

        // send_message();
    }
}

void tcp_data_transfer(void* parameters) {
    client_data *client_socket = parameters;
    int recv_buff_size = kilobytes(64);
    client_socket->recv_buff = malloc(recv_buff_size);
    void* recv_data = client_socket->recv_buff;

    int send_buff_size = kilobytes(64);
    client_socket->send_buff = malloc(send_buff_size);
    void* send_data = client_socket->send_buff;
    while(true) {
        client_socket->recv_size = recv(client_socket->client_id, client_socket->recv_buff, recv_buff_size, 0);
        if(client_socket->recv_size < 0) {
            ESP_LOGE(SOCKET_TAG, "Failed to receive data from the client socket id: %d, with errno: %d.", 
                                    client_socket->client_id, errno);
            ESP_LOGE(SOCKET_TAG, "Closing connection with the client_id: %d.", client_socket->client_id);
            close(client_socket->client_id);
            free(client_socket->recv_buff);
            free(client_socket->send_buff);
            free(client_socket);
            vTaskDelete(NULL);
        }

        // msg_type: 0 - default message,
        // msg_type: 1 - data request,
        // msg_type: 2 - data input,
        // ...
        // msg_type: 14 - restart,
        // msg_type: 15 - shutdown
        char msg_type = *((char *)recv_data);
        recv_data += sizeof(char);

        ESP_LOGD(SOCKET_TAG, "msg_type: %i", msg_type);

        switch(msg_type) {
            case 0: {
                ESP_LOGD(SOCKET_TAG, "%s", (char *)recv_data);
            } break;

            case 1: {
                ESP_LOGD(SOCKET_TAG, "Requested data!");
                // Requested data in the format of
                // name (0 delimiter) size (in bytes)
                // Setting the msg_type to data input
                *(char *)send_data = 2;
                send_data += sizeof(char);

                while(recv_data - client_socket->recv_buff < client_socket->recv_size) {
                    int name_length = 0;
                    int size = 0;
                    while(*((char *)recv_data + name_length)) {
                        name_length++;
                    }

                    name_length++;
                    char *var_name = malloc(name_length * sizeof(char));
                    memcpy(var_name, recv_data, name_length * sizeof(char));
                    recv_data += name_length * sizeof(char);

                    size = *(int *)recv_data;
                    recv_data += sizeof(int) + 1;

                    // Preparing the data to be sent
                    memcpy(send_data, var_name, name_length * sizeof(char));
                    send_data += name_length * sizeof(char);

                    // Determine which data point
                    DATA_POINTS data_point = 0;
                    while(strcmp(data_points_array[data_point], var_name)) {
                        data_point++;
                        if(data_point >= data_points_num) {
                            ESP_LOGW(SOCKET_TAG, 
                                "Client requested unavailable data point! client_id: %i, requested_name: %s", 
                                client_socket->client_id, var_name);
                            break;
                        }
                    }

                    switch(data_point) {
                        case GPIO0: {
                        } break;

                        case GPIO1: {
                        } break;

                        case GPIO2: {
                        } break;

                        case GPIO3: {
                        } break;

                        case GPIO4: {
                            // for testing purposes now
                            int value = gpio_get_level(GPIO_NUM_4);
                            memset(send_data, value, 1);
                            send_data += size;
                        } break;

                        default: {
                            // Unknown data point sending 0 value
                            memset(send_data, 0, size);
                            send_data += size;
                        } break;
                    }

                    *(char *)send_data = 0;
                    send_data += sizeof(char);
                }

                // Send the message back to the client
                int send_size = send_data - client_socket->send_buff;
                send_size = send(client_socket->client_id, client_socket->send_buff, send_size, 0);
            } break;

            case 14: {
                ESP_LOGD(SOCKET_TAG, "Received a restart message, restarting!");
                // TODO: restarting procedure
            } break;

            case 15: {
                ESP_LOGD(SOCKET_TAG, "Received a shutdown message, shutting down!");
                // TODO: shutting down procedure
            } break;

            default: {
                ESP_LOGW(SOCKET_TAG, "Unknown message type received!");
            } break;
        }
    }
}

void tcp_server_task() {
    int socket_id;
    struct sockaddr sock_addr;

    ESP_ERROR_CHECK(create_socket(&socket_id, AF_INET, SOCK_STREAM, IPPROTO_TCP));
    config_socket((struct sockaddr_in*) &sock_addr, AF_INET, PORT, IPADDR_ANY);
    ESP_ERROR_CHECK(bind_socket(socket_id, &sock_addr));
    ESP_ERROR_CHECK(listen_socket(socket_id));

    // TODO: When is this usefull/necessary?
    // int keep_alive = 1;

    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    socklen_t client_addrlen;
    char addr_str[128];
    while(true) {
        client_data* client_socket = malloc(sizeof(client_data));
        client_socket->client_id = accept(socket_id, (struct sockaddr *)&client_addr, &client_addrlen);
        if(client_socket->client_id == -1) {
            // TODO: Error Logging
            free(client_socket);
            client_socket = NULL;
        }

        if(client_socket) {

            inet_ntoa_r(client_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
            ESP_LOGD(SOCKET_TAG, "Accepted a connection from: %s", addr_str);

            // setsockopt(client_id, SOL_SOCKET, SO_KEEPALIVE, &keep_alive, sizeof(int));
            xTaskCreatePinnedToCore(tcp_data_transfer, "TCP_DATA_TRANSFER", STACK_SIZE, (void *)client_socket, tskIDLE_PRIORITY, NULL, tskNO_AFFINITY);
        }
    }
}