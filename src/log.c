
#include <time.h>
#include <stdio.h>

#include <glib.h>

static char *log_timestamp(void) {
    struct tm time_struct;
    time_t ts = time(NULL);

    gmtime_r(&ts, &time_struct);

    char formatted_time[256];
    int written = strftime(formatted_time, 255, "%Y%m%d-%H:%M:%S", &time_struct);
    if (written < 16) {
        return NULL;
    }

    char *timestamp = strdup(formatted_time);
    return timestamp;
}

void timestamp_prefix_log(void) {
    char *ts = log_timestamp();
    g_print("%s ", ts);
    free(ts);
}

void timestamp_prefix_err(void) {
    char *ts = log_timestamp();
    g_printerr("%s ", ts);
}