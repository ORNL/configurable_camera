#include <stdio.h>

#include <gst/gst.h>

#include "log.h"
#include "config.h"
#include "pipeline.h"

gboolean restart_file_limit_timer(PipelineData *pipeline_data) {
    if (!g_source_remove(pipeline_data->file_limit_callback_id)) {
        timestamp_prefix_err();
        g_printerr("Failed to remove the file limit timer callback.\n");
        return FALSE;
    }
    int time_limit = pipeline_data->control_data->rawTimeLimit;
    pipeline_data->file_limit_callback_id =
        g_timeout_add_seconds(time_limit, (GSourceFunc) file_limit, (gpointer) pipeline_data);
    return TRUE;
}

gboolean record_raw(PipelineData *pipeline_data, char *trigger_timestamp) {
    
    restart_file_limit_timer(pipeline_data);

    ControlData *control_data = pipeline_data->control_data;

    // format and create the recording directory  
    char *recording_dir;
    // config.h
    if (format_recording_directory(control_data, &recording_dir) == -1) {
    timestamp_prefix_err();
    g_printerr("Could not format recording directory.\n");
    free(recording_dir);
    return FALSE;
    }

    pipeline_data->recording_filepath = 
        g_strdup_printf("%s%s.raw", recording_dir, trigger_timestamp);

    g_signal_emit_by_name(pipeline_data->sink, "split-now");

    pipeline_data->is_raw_recording = TRUE;
    
    return TRUE;
}

gboolean stop_record_raw(PipelineData *pipeline_data) {

    restart_file_limit_timer(pipeline_data);

    pipeline_data->recording_filepath = g_strdup("/dev/null");
    g_signal_emit_by_name(pipeline_data->sink, "split-now");
    pipeline_data->is_raw_recording = FALSE;
    return TRUE;
}


gboolean new_csv(PipelineData *pipeline_data, char *filepath) {
  // creates a new csv by swapping filesink elements in the csv pipeline
  FILE *metadata_csv_file;
  metadata_csv_file = fopen(filepath, "w");
  fputs(METADATA_CSV_HEADER, metadata_csv_file);
  fclose(metadata_csv_file);

  gst_element_send_event(pipeline_data->csv_sink, gst_event_new_eos());
  gst_element_set_state(pipeline_data->csv_sink, GST_STATE_NULL);
  gst_bin_remove(GST_BIN(pipeline_data->csv_pipeline), pipeline_data->csv_sink);

  g_free(pipeline_data->csv_filepath);
  pipeline_data->csv_filepath = g_strdup(filepath);

  // TODO: check for sucessful make
  pipeline_data->csv_sink = gst_element_factory_make("filesink", "csv_sink");
  g_object_set(pipeline_data->csv_sink, "location", pipeline_data->csv_filepath, "append", TRUE, NULL);

  gst_bin_add(GST_BIN(pipeline_data->csv_pipeline), pipeline_data->csv_sink);
  gst_element_link(pipeline_data->csv_queue, pipeline_data->csv_sink);
  gst_element_set_state(pipeline_data->csv_sink, GST_STATE_PLAYING);

  // TODO: error handling
  return TRUE;
}

int frame_level_metadata(GstPad *src_pad, GstPadProbeInfo *info, gpointer user_data) {
  /*
  gets sensor info, formats a line of a csv, and pushes that data to a gstreamer csv writer pipeline
  */

  GstFlowReturn ret;

  PipelineData *pipeline_data = user_data;
  SensorDynamicInfo *sensor_info = getSensorDynamicInfo(pipeline_data->source);
  gchar *csv_line = formatSensorDynamicInfoCSV(sensor_info);
  freeSensorDynamicInfo(sensor_info);
  // creating buffer frees csv_line memory
  GstBuffer *csv_buffer = gst_buffer_new_wrapped(csv_line, sizeof(gchar) * strlen(csv_line));
  g_signal_emit_by_name(pipeline_data->appsrc, "push-buffer", csv_buffer, &ret);

  if (ret != GST_FLOW_OK) {
    timestamp_prefix_err();
    g_printerr("Failed to push frame metadata buffer to appsrc. Error code: %d\n", ret);
  }

  return GST_PAD_PROBE_OK;
}

gboolean file_limit(gpointer data) {
    // To be used with a timed callback
    // Ends raw recording, starts a new csv file for metadata
    PipelineData *pipeline_data = data;

    stop_record_raw(pipeline_data);

    time_t timestamp = time(NULL);

    // format the filepath used for the new csv
    char *recording_dir;
    // config.h
    if (format_recording_directory(pipeline_data->control_data, &recording_dir) == -1) {
        free(recording_dir);
        timestamp_prefix_err();
        g_printerr("Could not format recording directory when csv time limit was reached.\n");
        return G_SOURCE_REMOVE;
    }
    gchar *sensor_metadata_filepath = g_strdup_printf("%s%ld_metadata.csv", recording_dir,
        timestamp);

    timestamp_prefix_log();
    g_print("Metadata csv time limit reached. Starting a new one at %s.\n", 
        sensor_metadata_filepath);
    new_csv(pipeline_data, sensor_metadata_filepath);
    free(recording_dir);
    g_free(sensor_metadata_filepath);

    return G_SOURCE_CONTINUE;
}

void metadata_cleanup(gpointer user_data) {
  /*
    Swap out the current filesink in the csv recorder pipeline with a fakesink
    to close the file.
  */

  PipelineData *pipeline_data = user_data;

  gst_element_send_event(pipeline_data->csv_sink, gst_event_new_eos());
  gst_element_set_state(pipeline_data->csv_sink, GST_STATE_NULL);
  gst_bin_remove(GST_BIN(pipeline_data->csv_pipeline), pipeline_data->csv_sink);

  g_free(pipeline_data->csv_filepath);
  pipeline_data->csv_filepath = NULL;

  // TODO: check for sucessful make
  pipeline_data->csv_sink = gst_element_factory_make("fakesink", "csv_sink");

  gst_bin_add(GST_BIN(pipeline_data->csv_pipeline), pipeline_data->csv_sink);
  gst_element_link(pipeline_data->csv_queue, pipeline_data->csv_sink);
  gst_element_set_state(pipeline_data->csv_sink, GST_STATE_PLAYING);

}
