#include "yt/fetcher.h"
#include <curl/curl.h>
#include <gmodule.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "Plugin_YT"

int data_closure_init(DataClosure *mem) {
    mem->size = 0;
    mem->pos = 0;
    mem->data = malloc(mem->size + 1);
    if (mem->data == NULL) {
        return 1;
    }
    mem->data[0] = '\0';
    return 0;
}

void data_closure_free(DataClosure *mem) {
    if (mem->data) {
        free(mem->data);
        mem->data = NULL;
        mem->size = 0;
        mem->pos = 0;
    }
    free(mem);
}

cairo_status_t read_png_stream_from_byte_array(void *in_closure, unsigned char *data, unsigned int length) {
    DataClosure *closure = (DataClosure *)in_closure;
    if ((closure->pos + length) > (closure->size)) {
        return CAIRO_STATUS_READ_ERROR;
    }
    memcpy(data, (closure->data + closure->pos), length);
    closure->pos += length;
    return CAIRO_STATUS_SUCCESS;
}

size_t write_func(void *ptr, size_t size, size_t nmemb, DataClosure *mem) {
    size_t new_size = mem->size + size * nmemb;
    mem->data = realloc(mem->data, new_size + 1);
    if (mem->data == NULL) {
        fprintf(stderr, "realloc() failed\n");
        exit(EXIT_FAILURE);
    }
    memcpy(mem->data + mem->size, ptr, size * nmemb);
    mem->data[new_size] = '\0';
    mem->size = new_size;

    return size * nmemb;
}

DataClosure *fetch_url_contents(const char *url) {
    CURL *curl;
    CURLcode res;
    DataClosure *mem = malloc(sizeof(DataClosure));

    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if (curl) {
        data_closure_init(mem);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_func);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, mem);
        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return mem;
}
