#ifndef MAIN_H
#define MAIN_H

#define _GNU_SOURCE
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>

#define REQUEST_SIZE 1024        // request size
#define MAX_REQUEST_SIZE 2000    // Max req size to check in cache
#define RESPONSE_SIZE 1024       // response size for buffer
#define MAX_RESPONSE_SIZE 102400 // Max response size for !!caching!!
#define BREAK_LINE "\r\n\r\n"    // break line
#define SMALL_BREAK_LINE "\r\n"  // small break line
#define SMALL_BREAK_LINE_LEN 2   // small break line length
#define BREAK_LINE_LEN 4         // break line length
#define HOST_NAME_SIZE 64        // host name size
#define URL_SIZE 128             // url size
#define ALWAYS_RUNNING 1         // keep the server running
#define R_BREAK "\r"             
#define N_BREAK "\n"
#define DEFAULT_PORT "80"        // default port
#define HOST_LINE "Host:"        // host line
#define SIX 6                     // six
#define BUFFER_SIZE 8192          // max buffer size
#define MAX_ENTRIES 10            // max entries in cache
#define TEMP_BUFFER_SIZE 512      // temp buffer size for cache-control
#define CACHE_HIT -2              // cache hit

typedef struct {
    int request_len;                        // request length
    char request[MAX_REQUEST_SIZE + 1];     // request buffer
    char url[URL_SIZE];                     // url
    char host_name[HOST_NAME_SIZE];         // host name
    int is_stale;                           // mark as 1 if stale
} request_info_t;                           // serve as a buffer for request and response

// Structure to hold the cache entry
typedef struct {
    // key value store
    char key[MAX_REQUEST_SIZE + 1];    // Entire Request
    char value[MAX_RESPONSE_SIZE + 1]; // Response
    int value_size;                    // size of the response
    int is_present;                    // there is entry
    int time;                          // for LRU
    int max_age;                       // age of the cache entry
    time_t time_inserted;              // time when the entry was added
    char url[URL_SIZE];
    char host_name[HOST_NAME_SIZE];

} cache_entry_t;

typedef struct {
    int cache_enabled;                  // is cache enabled
    cache_entry_t entries[MAX_ENTRIES];
    int num_entries;                    // current entries in the cache
    int current_time;                   // current time for LRU
    request_info_t pending_info;        // pending info;
} cache_t;

// function for main
void parse_arguments(int argc, char **argv, char **port, cache_t *cache);

// function about socket
int listen_socket_creator(char *port);
int client_socket_creator(char *port, char *host_name);

// inportant fucntions for handle request and response
void running_process_handler(int socket_listener, char *port, cache_t *cache);
int request_handler(int message_socket, char *port, cache_t *cache, request_info_t *request_info);
void response_handler(int client_socket, int message_socket, cache_t *cache, request_info_t *request_info);

// functions for parsing request
void host_name_handler(char *host_header, char *host_name, char *host_port);
int get_last_line(const char *buffer, int len, const char **line_ptr, int *line_len);
char *format_host(char *host);

// functions for cache
void init_cache(cache_t *cache);
int search_cache(cache_t *cache, char *request, int request_len);
void insert_into_cache(cache_t *cache, request_info_t *info, const char *response, int response_len, int max_age);
int evict_with_LRU(cache_t *cache);
int cache_request_handler(cache_t *cache, char *buffer, int total_bytes, char *url, char *host_name, int message_socket, request_info_t *request_info);

// function for response
int is_cacheable(char *response_header);
int get_max_age(char *response_header);
int has_expired(cache_entry_t *entry);
void forward_response_body(int client_socket, int message_socket, char *response_header, char *full_response, int *response_len, int header_size, int body_size, int content_length);
int read_response(int client_socket, int message_socket, char *response_header, char *full_response, int *response_len);
void response_cache_handler(cache_t *cache, request_info_t *request_info, char *response_header, char *full_response, int response_len);

#endif