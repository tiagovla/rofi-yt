#include "yt/parser.h"
#include <regex.h>

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "Plugin_YT"

void parser_destroy_element(gpointer data) {
    if (data != NULL) {
        Match *entry = (Match *)data;
        g_free(entry->title);
        g_free(entry->url);
        g_free(entry);
    }
}

GPtrArray *match(char *sz) {
    GPtrArray *matches = g_ptr_array_new_with_free_func(parser_destroy_element);

    regex_t p1, p2;
    regmatch_t pmatch1[2], pmatch2[2];
    regcomp(&p1, "\\/watch?v=\\(.\\{11\\}\\)", 0);
    regcomp(&p2, "\\runs\":\\[{\"text\":\"\\([^\"]*\\)\"", 0);

    int eflags = 0, n_matches = 0;
    size_t offset = 0, length = strlen(sz);

    while (regexec(&p1, sz + offset, 2, pmatch1, eflags) == 0) {
        eflags = REG_NOTBOL;
        int s1 = offset + pmatch1[1].rm_so;
        int e1 = offset + pmatch1[1].rm_eo;
        offset += pmatch1[0].rm_eo;
        if (pmatch1[0].rm_so == pmatch1[0].rm_eo) {
            offset += 1;
        }
        if (offset > length) {
            break;
        }

        if (regexec(&p2, sz + offset, 2, pmatch2, eflags) == 0) {
            int s2 = offset + pmatch2[1].rm_so;
            int e2 = offset + pmatch2[1].rm_eo;

            Match *m = g_malloc(sizeof(Match));

            m->title = g_strndup(sz + s2, e2 - s2);
            m->url = g_strndup(sz + s1, e1 - s1);
            g_ptr_array_add(matches, m);

            n_matches++;
            if (n_matches >= 5) {
                break;
            }

            offset += pmatch2[0].rm_eo;
            if (pmatch2[0].rm_so == pmatch2[0].rm_eo) {
                offset += 1;
            }
            if (offset > length) {
                break;
            }
        }
    }
    regfree(&p1);
    regfree(&p2);
    return matches;
}
