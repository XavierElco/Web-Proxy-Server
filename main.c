#include "main.h"


int main(int argc, char **argv) {
    // handle the port and cache
    char *port = NULL;
    cache_t cache;

    // init empty cache
    init_cache(&cache);
    parse_arguments(argc, argv, &port, &cache);

    // create the listener socket
    int socket_listener = listen_socket_creator(port);

    // listen socket and handle req
    running_process_handler(socket_listener, port, &cache);

    return 0;
}


// parse the arguments
void parse_arguments(int argc, char **argv, char **port, cache_t *cache) {
    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "-p") == 0) {
            if (i + 1 < argc) {
                *port = argv[++i];
            }
        }
        if (strcmp(argv[i], "-c") == 0) {
            cache->cache_enabled = 1;
        }
        i++;
    }
}
