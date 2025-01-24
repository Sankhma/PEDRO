#include <Winsock2.h>
#include <ws2tcpip.h>

#define DEFAULT_IP "192.168.4.1"
#define DEFAULT_PORT "7777"

typedef struct {
    SOCKET connect_socket;
    struct addrinfo* server_address;
    struct addrinfo socket_type;
} client_socket;

typedef struct {
    char *buffer;
    int buffer_length;
    int bytes_to_transmit;
    int transmitted_bytes;
    int flags;
    char message_type;
} tcp_message;

client_socket create_socket() {
    int result = 0;
    client_socket client = {};
    client.socket_type.ai_family = AF_INET;
    client.socket_type.ai_socktype = SOCK_STREAM;
    client.socket_type.ai_protocol = IPPROTO_TCP;

    client.connect_socket = INVALID_SOCKET;

    result = getaddrinfo(DEFAULT_IP, DEFAULT_PORT, &client.socket_type, &client.server_address);
    if(result != 0) {
        return client;
    }

    client.connect_socket = socket(client.server_address->ai_family, client.server_address->ai_socktype, 
                                    client.server_address->ai_protocol);

    if(client.connect_socket == INVALID_SOCKET) {
        // Failed to create a socket
        // Use WSAGetLastError
        freeaddrinfo(client.server_address);
        return client;
    }

    return client;
}

void connect_socket(client_socket *client) {
    int result = 0;

    result = connect(client->connect_socket, client->server_address->ai_addr, (int)(client->server_address->ai_addrlen));
    if(result == SOCKET_ERROR) {
        // Failed to connect
        // Use WSAGetLastError
        closesocket(client->connect_socket);
        client->connect_socket = INVALID_SOCKET;
    }

    // This assumes we don't want to step through different server addresses (if there are any)
    freeaddrinfo(client->server_address);
}

void send_message(SOCKET connect_socket, tcp_message *message) {
    int result = 0;
    result = send(connect_socket, message->buffer, message->bytes_to_transmit, message->flags);
    if(result < 0) {
        // Failed to send the message
        // Use WSAGetLastError
        message->transmitted_bytes = 0;
    } else {
        message->transmitted_bytes = result;
    }
}

void receive_message(SOCKET connect_socket, tcp_message *message) {
    int result = 0;
    result = recv(connect_socket, message->buffer, message->buffer_length, message->flags);
    if(result < 0) {
        // Failed to receive message
        // Use WSAGetLastError
        message->transmitted_bytes = 0;
    } else {
        message->transmitted_bytes = result;
    }
}

void disconnect_socket(SOCKET connect_socket) {
    closesocket(connect_socket);
}

void send_data_request(SOCKET connect_socket, tcp_message *message, char **data_points, int num_data_points) {
    message->message_type = 1;

    message->bytes_to_transmit = 1;
    for(int i = 0; i < num_data_points; i++) {
        message->bytes_to_transmit += strlen(data_points[i]) + 1;
        message->bytes_to_transmit += 2;
    }

    if(message->buffer_length < message->bytes_to_transmit) {
        // Debug info
        message->bytes_to_transmit = 0;
    }

    if(message->bytes_to_transmit > 0) {
        message->buffer[0] = message->message_type;

        int offset = 1;
        for(int i = 0; i < num_data_points; i++) {
            strcpy(message->buffer + offset, data_points[i]);
            offset += strlen(data_points[i]) + 1;

            // For now setting it as an int
            *(message->buffer + offset) = sizeof(int);
            *(message->buffer + offset + 1) = 0;
            offset += 2;
        }
    }

    // Send the message
    send_message(connect_socket, message);
}

void network_cleanup() {}