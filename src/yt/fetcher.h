#ifndef YT_FETCHER_H
#define YT_FETCHER_H

#include <cairo.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    char *data;
    size_t size;
    size_t pos;
} DataClosure;

int data_closure_init(DataClosure *mem);
void data_closure_free(DataClosure *mem);
size_t write_func(void *ptr, size_t size, size_t nmemb, DataClosure *mem);
DataClosure *fetch_url_contents(const char *url);
cairo_status_t read_png_stream_from_byte_array(void *in_closure, unsigned char *data, unsigned int length);

#endif /* YT_FETCHER_H */
