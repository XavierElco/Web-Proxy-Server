#include "main.h"


// checck if response cacheable
int is_cacheable(char *response_header) {
    // def a variable for comparing
    char *start = strcasestr(response_header, "Cache-Control");

    if (!start) {
        return 1; // no Cache-Control header, assume cacheable
    }
    // get the cache_control line till they find \r\n
    // get start and end of the line
    start += strlen("Cache-Control");
    char *line_end = strstr(start, SMALL_BREAK_LINE);

    // store into temp buffer
    char buffer[TEMP_BUFFER_SIZE]; // assuming line follows format
    strncpy(buffer, start, line_end - start);
    buffer[line_end - start] = '\0';

    // implement case insensitive cmp
    if (strcasestr(buffer, "no-cache") || strcasestr(buffer, "no-store") ||
        strcasestr(buffer, "private") || strcasestr(buffer, "max-age=0") ||
        strcasestr(buffer, "must-revalidate") ||
        strcasestr(buffer, "proxy-revalidate")) {
        return 0;
    }

    return 1;
}


// get the max age from response header
int get_max_age(char *response_header) {
    char *start = strcasestr(response_header, "Cache-Control");
    if (!start) {
        return -1; // no Cache-Control header, assume will not expire
    }
    start += strlen("Cache-Control");

    char *max_age_ptr = strcasestr(start, "max-age=");

    if (max_age_ptr) {
        int max_age;
        // temporary buffer indorder to converting to lowercase
        char temp_buffer[32];
        strncpy(temp_buffer, max_age_ptr, sizeof(temp_buffer) - 1);
        temp_buffer[sizeof(temp_buffer) - 1] = '\0';
        
        // convert to lowercase
        for (int i = 0; temp_buffer[i] && i < 8; i++) {
            temp_buffer[i] = tolower(temp_buffer[i]);
        }
        
        if (sscanf(temp_buffer, "max-age=%d", &max_age) == 1) {
            return max_age; 
        }
    }

    return -1; // no max-age found
}


// check if cache entry has expired
int has_expired(cache_entry_t *entry) {
    if (entry->max_age == -1) {
        return 0; // no max-age, will not expire
    }
    time_t current_time = time(NULL);
    double age = difftime(current_time, entry->time_inserted);
    return age > entry->max_age;
}