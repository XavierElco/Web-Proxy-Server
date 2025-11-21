#include "main.h"


// handle the request and return the client socket
int request_handler(int message_socket, char *port, cache_t *cache, request_info_t *request_info) {
    char buffer[BUFFER_SIZE];
    int bytes_read;
    int total_bytes = 0;
    int header_complete = 0;

    // read the request
    while (total_bytes < BUFFER_SIZE - 1) {
        bytes_read = recv(message_socket, buffer + total_bytes, BUFFER_SIZE - 1 - total_bytes, 0);

        if (bytes_read <= 0) {
            if (bytes_read < 0) {
                perror("read from client failed in request_handler");
            }
            return -1;
        }

        total_bytes += bytes_read;
        buffer[total_bytes] = '\0';

        // if the buffer contains the break line, then the header is complete
        if (strstr(buffer, BREAK_LINE) != NULL) {
            header_complete = 1;
            break;
        }
    }

    if (!header_complete) {
        fprintf(stderr, "Request header too large or incomplete\n");
        return -1;
    }

    // get the last line
    const char *tail_ptr;
    int tail_len;

    if (get_last_line(buffer, total_bytes, &tail_ptr, &tail_len) == 0) {
        printf("Request tail %.*s\n", tail_len, tail_ptr);
        fflush(stdout);
    }

    // extract host name and port, so we can use the port and host name to
    // create a socket to connect to the server
    char host_name[HOST_NAME_SIZE];
    char host_port[6] = DEFAULT_PORT;
    char *host_header = strstr(buffer, HOST_LINE);
    host_name_handler(host_header, host_name, host_port);
    char url[URL_SIZE];
    sscanf(buffer, "%*s %s", url);


    // if request length<2000,
    if (cache->cache_enabled && total_bytes < MAX_REQUEST_SIZE) {
        int cache_result = cache_request_handler(cache, buffer, total_bytes, url, host_name, message_socket, request_info);
        if (cache_result == CACHE_HIT) {
            return CACHE_HIT; // response served from cache
        }

    }

    // create client socket if not found in cache
    int client_socket = client_socket_creator(host_port, host_name);

    /// there is getting the url
    if (client_socket > 0) {
        // extract the url
        if (strlen(url) > 0) {
            printf("GETting %s %s\n", host_name, url);
            // send the request to the server
            if (send(client_socket, buffer, total_bytes, 0) != total_bytes) {
                close(client_socket);
                return -1;
            }
        } else {
            close(client_socket);
            return -1;
        }
    } else {
        return -1;
    }

    return client_socket;
}