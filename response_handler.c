#include "main.h"


// this is a function that handles the response from the server and sends it
// to the client
void response_handler(int client_socket, int message_socket, cache_t *cache, request_info_t *request_info) {

    char response_header[RESPONSE_SIZE];
    char full_response[MAX_RESPONSE_SIZE];
    int response_len = 0;

    // read the response from the server
    read_response(client_socket, message_socket, response_header, full_response, &response_len);

    // cache the response
    response_cache_handler(cache, request_info, response_header, full_response, response_len);


    close(client_socket);
}


int read_response(int client_socket, int message_socket, char *response_header, char *full_response, int *response_len) {
    char buffer[BUFFER_SIZE];
    int header_complete = 0;
    int content_length = -1;
    int header_size = 0;
    int body_size = 0;
    int bytes_read;

    // printf("[DEBUG] Entering response handler...\n");
    // read and handle response
    while ((bytes_read = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        // append full response if it is cacheble
        if (*response_len + bytes_read <= MAX_RESPONSE_SIZE) {
            memcpy(full_response + *response_len, buffer, bytes_read);
        }
        *response_len += bytes_read;

        send(message_socket, buffer, bytes_read, 0);

        // while header is not complete, copy the header to the response_header
        if (!header_complete) {
            int copy_size = bytes_read;
            if (header_size + copy_size > RESPONSE_SIZE - 1) {
                copy_size = RESPONSE_SIZE - 1 - header_size;
            }

            // if copy_size is greater than 0, copy the buffer to the
            // response_header
            if (copy_size > 0) {
                memcpy(response_header + header_size, buffer, copy_size);
                header_size += copy_size;
                response_header[header_size] = '\0';
                char *header_end = strstr(response_header, BREAK_LINE);
                if (header_end) {
                    header_complete = 1;
                    *(header_end + BREAK_LINE_LEN) = '\0';

                    // read the Content-Length
                    char *content_ptr = strstr(response_header, "Content-Length:");
                    if (content_ptr) {
                        sscanf(content_ptr, "Content-Length: %d", &content_length);
                        printf("Response body length %d\n", content_length);
                        fflush(stdout);
                        int header_part_len = header_end + BREAK_LINE_LEN - response_header;
                        body_size = bytes_read - header_part_len;

                        // if body size is greater than content length, which means
                        // the response is complete, so break the loop
                        if (content_length > 0 && body_size >= content_length) {
                            break;
                        }
                    } else {
                        content_length = -1;
                    }
                }
            }
        } else if (content_length > 0) { // if read header is complete, then just read the body
            body_size += bytes_read;
            if (body_size >= content_length) {
                break;
            }
        }
    }
    return 0;
}


void response_cache_handler(cache_t *cache, request_info_t *request_info, char *response_header, char *full_response, int response_len) {
    // cache response if cache is enabled and res and request < max size
    //  check if cacheable
    if (cache->cache_enabled && request_info->request_len < MAX_REQUEST_SIZE &&
        response_len < MAX_RESPONSE_SIZE) {
        if (is_cacheable(response_header)) {
            insert_into_cache(cache, request_info, full_response, response_len, get_max_age(response_header));
        } else {
            // not cacheable
            printf("Not caching %s %s\n", request_info->host_name, request_info->url);
            fflush(stdout);

            // stale entry eviction for non-cacheable responses
            if (request_info->is_stale) {
                int index = search_cache(cache, request_info->request, request_info->request_len);
                if (index != -1) {
                    cache->entries[index].is_present = 0;
                    cache->num_entries--;
                    printf("Evicting %s %s from cache\n", request_info->host_name, request_info->url);
                    fflush(stdout);
                }
            }
        }
    } else if (cache->cache_enabled && request_info->is_stale) {

        // print stale evicted entries
        int index = search_cache(cache, request_info->request, request_info->request_len);
        if (index != -1) {
            cache->entries[index].is_present = 0;
            cache->num_entries--;
            printf("Evicting %s %s from cache\n", request_info->host_name, request_info->url);
            fflush(stdout);
        }
    }
}