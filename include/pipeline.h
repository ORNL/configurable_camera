#ifndef PIPELINE_H
#define PIPELINE_H

#include <gio/gio.h>
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

#include "sensor.h"
#include "config.h"

#define TXT_EXT ".txt"
#define RAW_EXT ".raw"
#define NEW_YEARS_2023 1672531200
#define STARTUP_METADATA "startup.json"
#define SENSOR_METADATA "sensor.json"
#define FRAME_METADATA "metadata.csv"

typedef struct PipelineData {
  GMainLoop *loop;

  // video pipeline elements 
  GstRTSPMedia *media;
  // TODO: Look into gst_rtsp_media_take_pipeline
  GstElement *media_bin;
  GstElement *source;
  GstElement *converter_capsfilter;
  GstElement *raw_queue;
  GstElement *sink;
  gchar *recording_filepath;
  int raw_recording_element;

  // csv pipeline elements
  GstElement *csv_pipeline;
  GstElement *appsrc;
  GstElement *csv_queue;
  GstElement *csv_sink;
  gchar *csv_filepath;

  GIOChannel *udp_channel;

  // IDs for adding and removing pipeline things
  guint file_limit_callback_id;
  guint udp_channel_watch_id;
  gulong metadata_probe_id;
  
  // pipeline monitoring things -- not gstreamer best practice, but fine for now
  // TODO: monitor the pipeline from the media bin or GstBus directly
  // TODO: Add a GstBus
  gint is_streaming;
  gint is_raw_recording;

  GstBus *bus;

  // data that needs to be passed around
  SensorSetupInfo *sensor_setup_info;
  ControlData *control_data;

} PipelineData;

gboolean restart_file_limit_timer(PipelineData *pipeline_data);

gboolean file_limit(gpointer data);

gboolean new_csv(PipelineData *pipeline_data, char *filepath);

gboolean record_raw(PipelineData *pipeline_data, char *trigger_timestamp);

gboolean stop_record_raw(PipelineData *pipeline_data);

int frame_level_metadata(GstPad *src_pad, GstPadProbeInfo *info, gpointer user_data);

void metadata_cleanup(gpointer user_data);

#endif
