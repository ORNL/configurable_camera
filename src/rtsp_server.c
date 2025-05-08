#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <glib.h>
#include <gio/gio.h>
#include <gst/gst.h>
#include <gst/gstbuffer.h>
#include <gst/rtsp-server/rtsp-server.h>

#include "log.h"
#include "sensor.h"
#include "pipeline.h"
#include "udp_server.h"
#include "rtsp_server.h"

#define RAW_TIMEOUT 30

static int num_clients = 0;

void cleanup_RTSP_media(PipelineData *pipeline_data) {

	timestamp_prefix_log();
  	g_print("RTSP client disconnected, stopping pipeline until a client connects\n");

  	pipeline_data->is_streaming = FALSE;
  	pipeline_data->media = NULL;

	stop_record_raw(pipeline_data);

 	// removing the metadata pad probe
 	if (pipeline_data->metadata_probe_id != 0) {
   		GstPad *src_pad = gst_element_get_static_pad(pipeline_data->source, "src");
    	gst_pad_remove_probe(src_pad, pipeline_data->metadata_probe_id);
    	gst_object_unref(src_pad);
    	pipeline_data->metadata_probe_id = 0;
        timestamp_prefix_log();
        g_print("Removed frame-level metadata probe.\n");
 	}

	// stopping normal raw trigger udp processing
  	if (pipeline_data->udp_channel_watch_id != 0) {
    	if (!g_source_remove(pipeline_data->udp_channel_watch_id)) {
      		timestamp_prefix_err();
      		g_printerr("Failed to find/remove the UDP listener source during stream cleanup.\n");
    	}
   		pipeline_data->udp_channel_watch_id = 0;
        timestamp_prefix_log();
        g_print("Removed UDP trigger callback.\n");
    }

    if (pipeline_data->file_limit_callback_id != 0) {
        if (!g_source_remove(pipeline_data->file_limit_callback_id)) {
            timestamp_prefix_err();
            g_printerr("Failed to find/remove the file limit timer during stream cleanup.\n");
        }
        pipeline_data->file_limit_callback_id = 0;
        timestamp_prefix_log();
        g_print("Removed file time limit callback.\n");
    }
}

void client_disconnected(GstRTSPClient *client, gpointer user_data) {
    GstRTSPUrl *url = user_data;
    timestamp_prefix_log();
    g_print("Client %s:%i closed. Clients: %i\n", url->host, url->port, --num_clients);
}

void client_connected(GstRTSPServer *server, GstRTSPClient *client, gpointer user_data) {
    // Call the client filter function to get information from a client when it is connected
    GstRTSPConnection *conn = gst_rtsp_client_get_connection(client);
    GstRTSPUrl *url = gst_rtsp_connection_get_url(conn);
    timestamp_prefix_log();
    g_print("Client connected: %s:%i. Clients: %i\n", url->host, url->port, ++num_clients);
    g_signal_connect(client, "closed", (GCallback)client_disconnected, url);
 }

gchararray format_recording_filepath(GstElement *splitmux, guint fragment_id,
        gpointer pipeline_data) {
    PipelineData *pd = pipeline_data;
    return g_strdup(pd->recording_filepath);
}

void media_configure(GstRTSPMediaFactory *factory, GstRTSPMedia *media,
                     gpointer user_data) {
    // Callback intended to be called when RTSP media needs to be constructed,
    // I.E. when an RTSP request is made from a client. Use g_signal_connect() to
    // attach the "media-configure" signal to this callback
    timestamp_prefix_log();
    g_print("Setting up RTSP media.\n");
    
    PipelineData *pipeline_data = user_data;
    SensorStaticInfo *ssi;
    
    pipeline_data->media = media;
    
    GstPad *src_pad;
    GstElement *media_bin;
    
    // make sure the pipeline data is freed
    g_object_set_data_full(G_OBJECT(media), "rtsp-extra-data", pipeline_data,
        (GDestroyNotify)cleanup_RTSP_media);

    /* get the element (bin) used for providing the streams of the media */
    pipeline_data->media_bin = media_bin = gst_rtsp_media_get_element(media);

    // set up the sensor, get the setup information
    pipeline_data->source = 
        gst_bin_get_by_name_recurse_up(GST_BIN(media_bin), "source");

    ssi = getSensorStaticInfo(pipeline_data->source);

    if (ssi == NULL) {
        timestamp_prefix_err();
        g_printerr("Sensor connection failed.\n");
        goto cleanup;
    }
    if (strcmp(ssi->sensor_setup_info->name,
        pipeline_data->sensor_setup_info->name) != 0) {
        timestamp_prefix_err();
        g_printerr("Current sensor has not been set up. Please run sensor_setup.\n");
        goto cleanup_ssi;
    }


    // get the raw recording elements if they exist
    if (pipeline_data->control_data->recordRaw) {
        pipeline_data->sink =
            gst_bin_get_by_name_recurse_up(GST_BIN(media_bin), "sink");
    
        // allow the location of the splitmuxsink to be changed by signal
        g_signal_connect(pipeline_data->sink, "format-location",
            (GCallback)format_recording_filepath, pipeline_data);

        pipeline_data->raw_queue =
            gst_bin_get_by_name_recurse_up(GST_BIN(media_bin), "raw_queue");
    } else {
        pipeline_data->sink = NULL;
        pipeline_data->raw_queue = NULL;
    }

    // listen for incoming UDP packets
    pipeline_data->udp_channel_watch_id = 
        g_io_add_watch(pipeline_data->udp_channel, G_IO_IN, 
        (GIOFunc)udp_received_callback, pipeline_data);

    /*
    get filepaths for the sensor and frame-level metadata
    then create those files
    */ 
    char *recording_dir = NULL;
    if (format_recording_directory(pipeline_data->control_data, &recording_dir) != 0) {
        timestamp_prefix_err();
        g_printerr("Failed to aquire recording directory.\n");
        goto cleanup_ssi;
    }

    time_t timestamp = time(NULL);

    char *sensor_metadata_filepath = g_strdup_printf("%s%ld_%s", recording_dir, timestamp, SENSOR_METADATA);

    char *sensor_setup_json = sensorStaticInfoJSON(ssi);

    FILE *sensor_metadata_file;
    sensor_metadata_file = fopen(sensor_metadata_filepath, "w");
    if (sensor_metadata_file == NULL) {
        timestamp_prefix_err();
        g_printerr("Failed to open sensor metadata file.\n");
        goto cleanup_sensor_metadata;
    }
    int rc = fputs(sensor_setup_json, sensor_metadata_file);
    if (rc == EOF) {
        timestamp_prefix_err();
        g_printerr("Failed to write sensor setup json to recording directory.\n");
        goto cleanup_sensor_metadata;
    }

    // free all the sensor setup metadata file stuff
    fclose(sensor_metadata_file);
    free(sensor_setup_json);
    sensor_setup_json = NULL;
    g_free(sensor_metadata_filepath);
    sensor_metadata_filepath = NULL;
    freeSensorStaticInfo(ssi);
    ssi = NULL;

    char *frame_metadata_filepath = g_strdup_printf("%s%ld_%s", recording_dir, timestamp, FRAME_METADATA);
    free(recording_dir);
    recording_dir = NULL;
    if (frame_metadata_filepath == NULL) {
        timestamp_prefix_err();
        g_printerr("Failed to format frame level metadata filepath.\n");
        // everything above this should be free now
        goto cleanup;
    }

    new_csv(pipeline_data, frame_metadata_filepath);

    // start the file limit timer
    int time_limit = pipeline_data->control_data->rawTimeLimit;
    pipeline_data->file_limit_callback_id = g_timeout_add_seconds(time_limit, 
        (GSourceFunc) file_limit, (gpointer) pipeline_data);

    free(frame_metadata_filepath);
    frame_metadata_filepath = NULL;

    // install the frame-level metadata probe at the source element

    src_pad = gst_element_get_static_pad(pipeline_data->source, "src"); 
    pipeline_data->metadata_probe_id = gst_pad_add_probe(src_pad, GST_PAD_PROBE_TYPE_BUFFER,
        (GstPadProbeCallback) frame_level_metadata, (gpointer) pipeline_data, 
        (GDestroyNotify) metadata_cleanup);

    gst_object_unref(src_pad);

    // TODO: gstreamer best practice is to monitor pipeline state with the pipeline itself, not a separate variable
    pipeline_data->is_streaming = TRUE;


    timestamp_prefix_log();
    g_print("Setup RTSP media.\n");

    return;

    cleanup_sensor_metadata:
        free(sensor_setup_json);
        sensor_setup_json = NULL;
        g_free(sensor_metadata_filepath);
        sensor_metadata_filepath = NULL;
    cleanup_recording_dir:
        free(recording_dir);
        recording_dir = NULL;
    cleanup_ssi:
        freeSensorStaticInfo(ssi);
    cleanup:
        g_main_loop_quit(pipeline_data->loop);
}
