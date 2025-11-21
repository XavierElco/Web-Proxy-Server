#include "main.h"


//keep listeiing in order to continue receiving request and resposne
void running_process_handler(int socket_listener, char *port, cache_t *cache) {
    // always runing to accept new connection
    while (ALWAYS_RUNNING) {

        // client address
        struct sockaddr_storage client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        // server socket
        int message_socket = accept(
            socket_listener, (struct sockaddr *)&client_addr, &client_addr_len);

        if (message_socket < 0) {
            continue;
        }

        printf("Accepted\n");
        fflush(stdout);

        // handle req and return client socket
        request_info_t request_info = {0};
        int client_socket =
            request_handler(message_socket, port, cache, &request_info);

        if (client_socket > 0) {
            // handle res from service and send it out
            response_handler(client_socket, message_socket, cache, &request_info);
        } else if (client_socket == -2) {
            // response served from cache
            close(message_socket);
        } else {
            printf("something went wrong\n");
        }

        close(message_socket);
    }
}