#include "yt/fetcher.h"
#include "yt/parser.h"
#include "yt/utils.h"
#include <cairo.h>
#include <cairo_jpg.h>
#include <gmodule.h>
#include <pthread.h>
#include <rofi/helper.h>
#include <rofi/mode-private.h>
#include <rofi/mode.h>
#include <stdio.h>

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "Plugin_YT"

G_MODULE_EXPORT Mode mode;

typedef struct {
    gchar *title;
    gchar *url;
    cairo_surface_t *image;
} Entry;

typedef struct {
    gchar *current_query;
    gchar *next_query;
    gboolean fetching;
    GPtrArray *entries;
} YTModePrivateData;

void destroy_element(gpointer data) {
    if (data != NULL) {
        Entry *entry = (Entry *)data;
        g_free(entry->title);
        g_free(entry->url);
        cairo_surface_destroy(entry->image);
        g_free(entry);
    }
}

static void get_yt(Mode *sw) {
    YTModePrivateData *pd = (YTModePrivateData *)mode_get_private_data(sw);
    pd->fetching = FALSE;
    pd->current_query = g_strdup("");
    pd->next_query = g_strdup("");
    pd->entries = g_ptr_array_new_with_free_func(destroy_element);
}

static int yt_mode_init(Mode *sw) {
    if (mode_get_private_data(sw) == NULL) {
        YTModePrivateData *pd = g_malloc0(sizeof(*pd));
        mode_set_private_data(sw, (void *)pd);
        get_yt(sw);
    }
    return TRUE;
}

static unsigned int yt_mode_get_num_entries(const Mode *sw) {
    const YTModePrivateData *pd = (const YTModePrivateData *)mode_get_private_data(sw);
    return pd->entries->len;
}


static ModeMode yt_mode_result(Mode *sw, int menu_entry, char **input, unsigned int selected_line) {
    ModeMode retv = MODE_EXIT;
    YTModePrivateData *pd = (YTModePrivateData *)mode_get_private_data(sw);

    char buffer[8];
    int2hex(menu_entry, buffer, 8);
    g_debug("menu_entry: %s", buffer);

    g_debug("Result: entry:%d line:%d input:%s", menu_entry, selected_line, input[0]);
    if (menu_entry & MENU_NEXT) {
        retv = NEXT_DIALOG;
    } else if (menu_entry & MENU_PREVIOUS) {
        retv = PREVIOUS_DIALOG;
    } else if (menu_entry & MENU_QUICK_SWITCH) {
        retv = (menu_entry & MENU_LOWER_MASK);
    } else if ((menu_entry & MENU_OK)) {
        retv = RELOAD_DIALOG;
    } else if ((menu_entry & MENU_ENTRY_DELETE) == MENU_ENTRY_DELETE) {
        retv = RELOAD_DIALOG;
    } else if (menu_entry & MENU_CUSTOM_INPUT) {
        gchar *prefix = "https://www.youtube.com/results?search_query=";
        gchar *suffix = "&sp=EgIQAQ%253D%253D";

        gchar *url = g_markup_printf_escaped("%s%s%s", prefix, input[0], suffix);
        DataClosure *dc = fetch_url_contents(url);
        g_debug("size: %zu", dc->size);

        GPtrArray *matches = match(dc->data);

        while (pd->entries->len > 0)
            g_ptr_array_remove_index(pd->entries, 0);

        for (int i = 0; i < matches->len; i++) {
            Match *match = g_ptr_array_index(matches, i);
            gchar *url_image = g_markup_printf_escaped("https://i.ytimg.com/vi/%s/hqdefault.jpg", match->url);

            DataClosure *di = fetch_url_contents(url_image);
            cairo_surface_t *surface = cairo_image_surface_create_from_jpeg_mem(di->data, di->size);
            g_debug("%s", url_image);
            Entry *e = g_malloc(sizeof(Entry));
            e->title = g_strdup(match->title);
            e->url = g_strdup(match->url);
            // e-> image = NULL;
            e->image = surface;
            // data_closure_free(di);
            g_free(url_image);
            g_ptr_array_add(pd->entries, e);
        }

        g_free(url);
        g_ptr_array_free(matches, TRUE);
        data_closure_free(dc);

        retv = RELOAD_DIALOG;
    }

    return retv;
}

static void yt_mode_destroy(Mode *sw) {
    YTModePrivateData *pd = (YTModePrivateData *)mode_get_private_data(sw);
    if (pd != NULL) {
        g_free(pd->current_query);
        g_free(pd->next_query);
        g_ptr_array_free(pd->entries, TRUE);
        g_free(pd);
        mode_set_private_data(sw, NULL);
    }
}

static char *yt_get_display_value(const Mode *sw, unsigned int selected_line, G_GNUC_UNUSED int *state,
                                  G_GNUC_UNUSED GList **attr_list, int get_entry) {
    YTModePrivateData *pd = (YTModePrivateData *)mode_get_private_data(sw);

    Entry *res = g_ptr_array_index(pd->entries, selected_line);
    return get_entry ? g_strdup(res->title) : NULL;
}

static int yt_token_match(const Mode *sw, rofi_int_matcher **tokens, unsigned int index) {
    // YTModePrivateData *pd = (YTModePrivateData *)mode_get_private_data(sw);
    // Entry *res = g_ptr_array_index(pd->entries, index);
    return TRUE;
}

static char *yt_get_message(const Mode *sw) { return g_markup_printf_escaped("Results:"); }

void *thread_worker(void *sw) {
    YTModePrivateData *pd = (YTModePrivateData *)mode_get_private_data((Mode *)sw);

    do {
        // TODO: live search
        // while (pd->entries->len > 0) {
        //   g_debug("removing entries");
        //   g_ptr_array_remove_index(pd->entries, 0);
        // }
        pd->current_query = g_strdup(pd->next_query);
    } while (g_strcmp0(pd->current_query, pd->next_query) != 0);

    pd->fetching = FALSE;
    return NULL;
}

static char *yt_preprocess_input(Mode *sw, const char *input) {
    YTModePrivateData *pd = (YTModePrivateData *)mode_get_private_data(sw);
    pd->next_query = g_strdup(input);

    if (pd->fetching == FALSE & g_strcmp0(pd->next_query, pd->current_query) != 0) {
        pd->fetching = TRUE;
        g_free(pd->next_query);
        pthread_t thread;
        pthread_create(&thread, 0, thread_worker, (void *)sw);
        pthread_detach(thread);
    }

    pd->current_query = g_strdup(input);
    return g_markup_printf_escaped("%s", input);
}

static char *_get_completion(const Mode *sw, unsigned int index) { return g_markup_printf_escaped("noicehoho"); }

static cairo_surface_t *yt_get_icon(const Mode *sw, unsigned int selected_line, unsigned int height) {

    YTModePrivateData *pd = (YTModePrivateData *)mode_get_private_data(sw);
    Entry *res = g_ptr_array_index(pd->entries, selected_line);
    return res->image;
}

Mode mode = {
    .abi_version = ABI_VERSION,
    .name = "yt",
    .cfg_name_key = "display-yt",
    ._init = yt_mode_init,
    ._get_num_entries = yt_mode_get_num_entries,
    ._result = yt_mode_result,
    ._destroy = yt_mode_destroy,
    ._token_match = yt_token_match,
    ._get_display_value = yt_get_display_value,
    ._get_message = yt_get_message,
    ._get_completion = _get_completion,
    ._preprocess_input = yt_preprocess_input,
    ._get_icon = yt_get_icon,
    .private_data = NULL,
    .free = NULL,
};
