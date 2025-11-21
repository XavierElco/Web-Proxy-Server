#include "main.h"


// create and connect socket for client. this function is for create a socket
// to connect to the server
int client_socket_creator(char *port, char *host_name) {

    int socket_fd, getaddr_status;
    struct addrinfo addrinfo_hints, *server_info, *server_info_pointer;

    // init
    memset(&addrinfo_hints, 0, sizeof(addrinfo_hints));
    addrinfo_hints.ai_family = AF_UNSPEC;     // IPv6 and IPv4
    addrinfo_hints.ai_socktype = SOCK_STREAM; // TCP

    // get address info from host
    getaddr_status =
        getaddrinfo(host_name, port, &addrinfo_hints, &server_info);

    if (getaddr_status != 0) {
        fprintf(stderr, "[ERROR] getaddrinfo(%s:%s): %s\n", host_name, port,
                gai_strerror(getaddr_status));
        return -1;
    }

    // iterate the address info list, trying to create and connect socket
    for (server_info_pointer = server_info; server_info_pointer != NULL;
         server_info_pointer = server_info_pointer->ai_next) {
        socket_fd = socket(server_info_pointer->ai_family,
                           server_info_pointer->ai_socktype,
                           server_info_pointer->ai_protocol);

        if (socket_fd == -1) {
            perror("socket");
            continue;
        }

        // trying connect to the server, if success, break the loop, otherwise
        // close the socket
        if (connect(socket_fd, server_info_pointer->ai_addr,
                    server_info_pointer->ai_addrlen) == 0) {
            break;
        }
        close(socket_fd);
        socket_fd = -1;
    }
    freeaddrinfo(server_info);

    // see if it is connected
    if (socket_fd == -1) {
        fprintf(stderr, "client:Failed to connect\n");
        exit(EXIT_FAILURE);
    }

    return socket_fd;
}


// create and bind socket for listening this is for the server to listen to
// the client
int listen_socket_creator(char *port) {

    int socket_fd, reuse_addr = 1, getaddr_status;
    struct addrinfo addrinfo_hints, *addrinfo_pointer, *addrinfo_res;

    memset(&addrinfo_hints, 0, sizeof(addrinfo_hints));
    addrinfo_hints.ai_family = AF_INET6;      // IPv4 or IPv6
    addrinfo_hints.ai_socktype = SOCK_STREAM; // TCP
    addrinfo_hints.ai_flags = AI_PASSIVE;     // for binding and listening

    getaddr_status = getaddrinfo(NULL, port, &addrinfo_hints, &addrinfo_res);

    // if return value is not zero, error
    if (getaddr_status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(getaddr_status));
        exit(EXIT_FAILURE);
    }

    // iterate the address info list, trying to create and bind socket
    for (addrinfo_pointer = addrinfo_res; addrinfo_pointer != NULL; addrinfo_pointer = addrinfo_pointer->ai_next) {
        socket_fd = socket(addrinfo_pointer->ai_family, addrinfo_pointer->ai_socktype, addrinfo_pointer->ai_protocol);

        // if failed on cur position, try next position
        if (socket_fd == -1) {
            continue;
        }

        setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));

        // if bind success, the loop will break, otherwise close it
        if (bind(socket_fd, addrinfo_pointer->ai_addr, addrinfo_pointer->ai_addrlen) == 0) {
            break;
        }
        close(socket_fd);
        socket_fd = -1;
    }

    if (socket_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // if listen success, return socket_fd, otherwise error
    if (listen(socket_fd, 10) == -1) {
        perror("listen");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(addrinfo_res);
    return socket_fd;
}


// this function is for extracting the host name and port from the host header
void host_name_handler(char *host_header, char *host_name, char *host_port) {
    if (host_header) {
        char full_host[HOST_NAME_SIZE + SIX];

        if (sscanf(host_header, "Host: %s", full_host) == 1) {
            // we need to get rid of the brackets to handle the ipv6 address
            char *formatted_host = format_host(full_host);

            strncpy(host_name, formatted_host, HOST_NAME_SIZE - 1);
            host_name[HOST_NAME_SIZE - 1] = '\0';

            if (formatted_host != full_host) {
                free(formatted_host);
            }
        }
    }
}


// get the last line of the request
int get_last_line(const char *buffer, int len, const char **line_ptr, int *line_len) {
    if (!buffer || len <= 0 || !line_ptr || !line_len) {
        return -1;
    }

    // find \r\n\r\n
    char *header_end = NULL;
    for (int i = 0; i <= len - BREAK_LINE_LEN; i++) {
        if (memcmp(buffer + i, BREAK_LINE, BREAK_LINE_LEN) == 0) {
            header_end = (char *)buffer + i;
            break;
        }
    }

    if (!header_end) {
        return -1;
    }

    char *last_rn = NULL;
    for (char *p = header_end - 2; p >= buffer; p--) {
        if (p[0] == '\r' && p[1] == '\n') {
            last_rn = p;
            break;
        }
    }

    if (!last_rn) {
        return -1;
    }

    *line_ptr = last_rn + 2;
    *line_len = header_end - *line_ptr;

    return 0;
}


// get the bracketed off
char *format_host(char *host) {
    size_t len = strlen(host);
    if (len >= 2 && host[0] == '[' && host[len - 1] == ']') {
        char *clean = malloc(len - 1);
        strncpy(clean, host + 1, len - 2);
        clean[len - 2] = '\0';
        return clean;
    }

    return host;
}