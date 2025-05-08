
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <gst/gst.h>
#include <gst/gstbuffer.h>

#include "pipeline.h"
#include "sensor.h"


gboolean create_csv(gpointer data) {
    PipelineData *pd = data;

    char filepath[1024];
    static int num = 0;
    sprintf(filepath, "~/metadata_test%d.csv", num++);
    printf("New CSV: %s.\n", filepath);
    new_csv(pd, filepath);

    return FALSE;
}

gboolean push_buffer(gpointer data) {
    PipelineData *pd = data;
    GstFlowReturn ret;
    gchar *header = strdup(METADATA_CSV_HEADER);
    GstBuffer *csv_buffer = gst_buffer_new_wrapped(header, sizeof(gchar) * strlen(header));
    printf("Pushing buffer to csv.\n");
    g_signal_emit_by_name(pd->appsrc, "push-buffer", csv_buffer, &ret);

    return TRUE;
}

int main(int argc, char* argv[]){
    
    PipelineData pipeline_data;

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);

    gst_init(&argc, &argv);

    pipeline_data.csv_pipeline = gst_pipeline_new("csv_pipeline");
    pipeline_data.appsrc = gst_element_factory_make("appsrc", "metadata_appsrc");
    pipeline_data.csv_queue = gst_element_factory_make("queue", "csv_queue");
    pipeline_data.csv_sink = gst_element_factory_make("fakesink", "csv_sink");
    pipeline_data.csv_filepath = NULL;
    if (!pipeline_data.csv_pipeline || !pipeline_data.appsrc || !pipeline_data.csv_queue || !pipeline_data.csv_sink) {
        g_error("Could not create necessary elements for the csv writer.\n");
        return -1;
    }
    gst_bin_add_many (GST_BIN (pipeline_data.csv_pipeline), pipeline_data.appsrc, pipeline_data.csv_queue, pipeline_data.csv_sink, NULL);
    gst_element_link_many(pipeline_data.appsrc, pipeline_data.csv_queue, pipeline_data.csv_sink, NULL);
    gst_element_set_state(pipeline_data.csv_pipeline, GST_STATE_PLAYING);

    g_timeout_add_seconds(1, create_csv, (gpointer) &pipeline_data);
    g_timeout_add_seconds(2, push_buffer, (gpointer) &pipeline_data);
    g_timeout_add_seconds(10, create_csv, (gpointer) &pipeline_data);

    g_main_loop_run(loop);
}
