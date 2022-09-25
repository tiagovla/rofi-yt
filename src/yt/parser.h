#ifndef YT_PARSER_H
#define YT_PARSER_H
#include <gmodule.h>

typedef struct {
    gchar *title;
    gchar *url;
} Match;

GPtrArray *match(char *sz);

#endif /* YT_PARSER_H */
