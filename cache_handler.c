#include "main.h"

// init cache
void init_cache(cache_t *cache) {
    cache->cache_enabled = 0;
    cache->num_entries = 0;
    cache->current_time = 0;
    
    for (int i = 0; i < MAX_ENTRIES; i++) {
        cache->entries[i].is_present = 0;
    }
}

// search for request in cache, return the index if found
int search_cache(cache_t *cache, char *request, int request_len) {
    for (int i = 0; i < MAX_ENTRIES; i++) {
        if (cache->entries[i].is_present &&
            memcmp(cache->entries[i].key, request, request_len) == 0) {
            return i;
        }
    }

    return -1;
}

// evicting pages
int evict_with_LRU(cache_t *cache) {
    int least_used_time = __INT_MAX__;
    int index = -1;

    for (int i = 0; i < MAX_ENTRIES; i++) {
        if (cache->entries[i].time < least_used_time) {
            least_used_time = cache->entries[i].time;
            index = i;
        }
    }

    if (index != -1) {
        cache->entries[index].is_present = 0;
    }

    return index;
}

// insert into cache
void insert_into_cache(cache_t *cache, request_info_t *info, const char *response, int response_len, int max_age) {
    int index = -1;

    // if stale, check if it is in cache
    if (info->is_stale) {
        index = search_cache(cache, info->request, info->request_len);
        if (index != -1) {
        }
    }
    

    if (index == -1) {
        for (index = 0; index < MAX_ENTRIES; index++) {
            if (!cache->entries[index].is_present) {
                cache->num_entries++;
                break;
            }
        }
    }

    // find empty index to insert
    if (index != -1 && index < MAX_ENTRIES) {
        // mark as present
        cache->entries[index].is_present = 1;
        cache->entries[index].value_size = response_len;

        // update accessed time
        cache->entries[index].time = ++(cache->current_time);

        // copy the request
        strncpy(cache->entries[index].key, info->request,info->request_len);
        cache->entries[index].key[info->request_len] = '\0';

        // copy the response
        memcpy(cache->entries[index].value, response, response_len);
        cache->entries[index].value[response_len] = '\0';

        // copy the host name
        strncpy(cache->entries[index].host_name, info->host_name,HOST_NAME_SIZE - 1);
        cache->entries[index].host_name[HOST_NAME_SIZE - 1] = '\0';

        // copy the url
        strncpy(cache->entries[index].url, info->url, URL_SIZE - 1);
        cache->entries[index].url[URL_SIZE - 1] = '\0';

        // set the time inserted
        cache->entries[index].time_inserted = time(NULL);
        cache->entries[index].max_age = max_age;

    }

}

int cache_request_handler(cache_t *cache, char *buffer, int total_bytes, char *url, char *host_name, int message_socket, request_info_t *request_info)  {
    
    // check if exist in cache
    int cache_index = search_cache(cache, buffer, total_bytes);

    // add to request info
    memcpy(request_info->request, buffer, total_bytes);
    request_info->request_len = total_bytes;
    strncpy(request_info->url, url, URL_SIZE - 1);
    request_info->url[URL_SIZE - 1] = '\0';
    strncpy(request_info->host_name, host_name, HOST_NAME_SIZE - 1);
    request_info->host_name[HOST_NAME_SIZE - 1] = '\0';

    int expired = 0;
    if (cache_index != -1) {
        expired = has_expired(&cache->entries[cache_index]);
    }
    // found and not expired
    if (cache_index != -1 && !expired) {
        // update the cache entry time
        cache->entries[cache_index].time = ++(cache->current_time);
        printf("Serving %s %s from cache\n", host_name, url);
        fflush(stdout);
        // If found in cache, use cached response
        send(message_socket, cache->entries[cache_index].value,
                cache->entries[cache_index].value_size, 0);

        return CACHE_HIT;
    } else if (cache_index != -1 && expired) {
        // found but expired
        request_info->is_stale = 1;
        printf("Stale entry for %s %s\n", host_name, url);
        fflush(stdout);
        // update the cache entry time
        cache->entries[cache_index].time = ++(cache->current_time);
    }
    // check if cache is full and evict no matter caching or not
    if (cache->num_entries >= MAX_ENTRIES) {
        // not found, check if cache is full, if full evict with LRU
        int evicted_index = evict_with_LRU(cache);
        printf("Evicting %s %s from cache\n",
                cache->entries[evicted_index].host_name,
                cache->entries[evicted_index].url);
        fflush(stdout);
        cache->num_entries--;
}

    return 0;
}