#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <gio/gio.h>
#include <gio/gnetworking.h>
#include <gst/gst.h>
#include "gst/gstbus.h"

#include "log.h"
#include "config.h"
#include "statusbroadcaster.h"
#include "pipeline.h"
#include "sensor.h"
#include "udp_server.h"
#include "rtsp_server.h"

#define PIPELINE_STRING_SIZE 2048

// TODO: I think this should have external linkage, probably shouldn't be static as it's passed into pipeline_data
// and potentially accessed elsewhere
static GMainLoop *loop;

// ---------- setup g_main loop exit on Ctrl-C ----------
void handleSIGINT(int junk) {
    timestamp_prefix_log();
    g_print("SIGINT -- exiting.\n");
    g_main_loop_quit(loop);
}

static gboolean rtsp_session_pool_cleanup(GstRTSPServer *server) {
	GstRTSPSessionPool *pool;
	pool = gst_rtsp_server_get_session_pool(server);
	gst_rtsp_session_pool_cleanup(pool);
	g_object_unref(pool);
	return TRUE;
}


int main(int argc, char* argv[]){

    timestamp_prefix_log();
    g_print("Starting camera program.\n");

    int err_code = EXIT_SUCCESS;

    gst_init(&argc, &argv);

	// ---------- Set up context structs and objects ----------

    ControlData *control_data = NULL;
    Metadata *metadata = NULL;
    SensorSetupInfo *sensor_setup_info;
    UDPListener *udp_listener;
    PipelineData pipeline_data;
    StatusBroadcast status_broadcast;

    loop = g_main_loop_new(NULL, FALSE);

    // Load sensor info from the setup file written by ./sensor_setup
    pipeline_data.sensor_setup_info = sensor_setup_info
        = loadSensorSetupInfo();

    if (sensor_setup_info == NULL) {
        timestamp_prefix_err();
        g_printerr("Failed to load sensor setup info.\n");
        err_code = EXIT_FAILURE;
        goto cleanup_loop;
    }

    // ---------- Read in config ---------- 
    if (readConfigFile(DEFAULT_CONFIG_FILEPATH, &control_data, &metadata) != 0) {
        timestamp_prefix_err();
        g_printerr("Couldn't read config file.\n");
        err_code = EXIT_FAILURE;
        goto cleanup_sensor_setup_info;
    }
    timestamp_prefix_log();
    g_print("Successfully read config file, verify values are correct:\n");
    printConfigSummary(control_data, metadata, control_data->head);


    // ---------- Format pipeline description ---------- 
    char *pipeline_string_format = gst_pipeline_txt_gen(DEFAULT_PIPELINE_FILEPATH, control_data->recordRaw);

    if (pipeline_string_format == NULL) {
        timestamp_prefix_err();
        g_printerr("Couldn't read pipeline setup file.\n");
        err_code = EXIT_FAILURE;
        goto cleanup_config;
    }

    char *bayer2rgb_with_link;

    if (sensor_setup_info->is_color) {
        // format in the bayer to rgb element if needed
        bayer2rgb_with_link = " bayer2rgb !";
    } else {
        // otherwise leave it alone, it's grayscale
        bayer2rgb_with_link = "";
    }
    // replace the %s in the string loaded from the text file with the sensor caps.
    // This is so sensor settings are set at pipeline creation.
    // format sensor settings, caps and bayer2rgb_with_link
	char *pipeline_string = g_strdup_printf(pipeline_string_format,
        sensor_setup_info->exposure_time,
        sensor_setup_info->exposure_auto,
        sensor_setup_info->gain,
        sensor_setup_info->gain_auto,
        sensor_setup_info->full_caps_string, bayer2rgb_with_link);

	if (pipeline_string == NULL) {
		timestamp_prefix_err();
		g_printerr("Failed to format pipeline string.\n");
		err_code = EXIT_FAILURE;
		goto cleanup_hardware_scripts;
	}

    g_print("Base pipeline:\n%s\n", pipeline_string);

    char *recording_dir;
    if (format_recording_directory(control_data, &recording_dir) != 0) {
        timestamp_prefix_err();
        g_printerr("Failed to create recording directory.\n");
        err_code = EXIT_FAILURE;
        goto cleanup_pipeline_string;
    }

    // format the start timestamp
    time_t ts = time(NULL);
	char *startup_metadata_filepath = g_strdup_printf("%s%ld_startup.json", recording_dir, ts);
    free(recording_dir);
    if (startup_metadata_filepath == NULL) {
        timestamp_prefix_err();
        g_printerr("Failed to format the startup metadata filename.\n");
        err_code = EXIT_FAILURE;
        goto cleanup_pipeline_string;
    }

    int n = copy_file(DEFAULT_CONFIG_FILEPATH, startup_metadata_filepath);
    g_free(startup_metadata_filepath);
    if (n != 0) {
        timestamp_prefix_err();
        g_printerr("Failed to copy startup config to metadata file.\n");
        err_code = EXIT_FAILURE;
        goto cleanup_pipeline_string;
    }

    // ---------- Start setting up the pipeline ---------- 

    pipeline_data.loop = loop;
    pipeline_data.control_data = control_data;
    pipeline_data.file_limit_callback_id = 0;
    pipeline_data.udp_channel_watch_id = 0;
    pipeline_data.metadata_probe_id = 0;
    pipeline_data.is_streaming = FALSE;
    pipeline_data.is_raw_recording = FALSE;
    pipeline_data.recording_filepath = NULL;

    // Set up ancillary metadata pipeline
    pipeline_data.csv_pipeline = gst_pipeline_new("csv_pipeline");
    pipeline_data.appsrc = gst_element_factory_make("appsrc", "metadata_appsrc");
    pipeline_data.csv_queue = gst_element_factory_make("queue", "csv_queue");
    pipeline_data.csv_sink = gst_element_factory_make("fakesink", "csv_sink");
    pipeline_data.csv_filepath = NULL;
    if (!pipeline_data.csv_pipeline || !pipeline_data.appsrc || !pipeline_data.csv_queue || !pipeline_data.csv_sink) {
        timestamp_prefix_err();
        g_printerr("Could not create necessary elements for the csv writer.\n");
        err_code = EXIT_FAILURE;
        goto cleanup_pipeline_string;
    }

    gst_bin_add_many (GST_BIN (pipeline_data.csv_pipeline), pipeline_data.appsrc, pipeline_data.csv_queue, pipeline_data.csv_sink, NULL);
    if (!gst_element_link_many(pipeline_data.appsrc, pipeline_data.csv_queue, pipeline_data.csv_sink, NULL)) {
        timestamp_prefix_err();
        g_printerr("CSV metadata pipeline elements could not be linked.\n");
        err_code = EXIT_FAILURE;
        goto cleanup_csv_pipeline;
    }

    gst_element_set_state(pipeline_data.csv_pipeline, GST_STATE_PLAYING);

    // ----------  Set up RTSP ---------- 

    GstRTSPServer *rtsp_server;
    GstRTSPMountPoints *rtsp_mount_points;
    GstRTSPMediaFactory *rtsp_media_factory;

    rtsp_server = gst_rtsp_server_new();
    gst_rtsp_server_set_address(rtsp_server, control_data->RTSPaddress);
    rtsp_mount_points = gst_rtsp_server_get_mount_points(rtsp_server);
    rtsp_media_factory = gst_rtsp_media_factory_new();
    
    gst_rtsp_media_factory_set_launch(rtsp_media_factory, pipeline_string);

    gst_rtsp_media_factory_set_shared(rtsp_media_factory, TRUE);

    g_signal_connect(rtsp_media_factory, "media-configure", (GCallback)media_configure, (gpointer) &pipeline_data);

    // the mount point will be configname_sensorname:
    // EX: rtsp://127.0.0.1:8554/config1_acA2040-120uc
    gchar *mount_point_str = g_strdup_printf("/%s_%s", metadata->name, sensor_setup_info->name);
    gst_rtsp_mount_points_add_factory(rtsp_mount_points, mount_point_str, rtsp_media_factory);
    g_object_unref(rtsp_mount_points);
    guint rtsp_server_id = gst_rtsp_server_attach(rtsp_server, NULL);

    timestamp_prefix_log();
    g_print("RTSP server ready at rtsp://%s:8554%s\n", control_data->RTSPaddress, mount_point_str);

    //g_signal_connect(rtsp_server, "client-connected", (GCallback)client_connected, (gpointer) &pipeline_data);
	g_timeout_add_seconds(2, (GSourceFunc)rtsp_session_pool_cleanup, rtsp_server);

    // ---------- Set up status broadcaster ----------
    status_broadcast.metadata = metadata;
    status_broadcast.pipeline_data = &pipeline_data;
    status_broadcast.sensor_info = NULL;

    if (!setup_socket(&status_broadcast)) {
        timestamp_prefix_err();
        g_printerr("Could not set up status broadcaster.\n");
        err_code = EXIT_FAILURE;
        goto cleanup_rtsp_server;
    }

    // ----------  Setup GIOChannel for incoming UDP ---------- 
    udp_listener = startUDPListener(control_data);
    if (udp_listener == NULL) {
        timestamp_prefix_err();
        g_printerr("Failed to start UDP listener channel.\n");
        err_code = EXIT_FAILURE;
        goto cleanup_status_broadcast;
    }

    pipeline_data.udp_channel = udp_listener->channel;


    // ------------------- Attach callbacks -------------------

    // broadcast relevant metadata, info, and pipeline status for monitoring
    guint status_broadcast_id = g_timeout_add_seconds(STATUS_TIMEOUT, status_broadcaster, (gpointer) &status_broadcast);

    // handle SIGINT by killing the main loop
    signal(SIGINT, handleSIGINT);

    // ---------- Fork Processes to call python hardware control scripts --------

    // Get number of hardware scripts to loop over
    gint number_of_hardware_scripts = 0;
    for (HardwareScripts *script_ptr = control_data->head; script_ptr != NULL; script_ptr = script_ptr->next){
        number_of_hardware_scripts++;
    }
    g_print("Number of hardware scripts: %d\n", number_of_hardware_scripts);
    
    // Malloc an array of pid_ts, use this array to kill child processes later
    pid_t *child_processes = malloc(sizeof(pid_t) * number_of_hardware_scripts);
    HardwareScripts *hw_script_ptr = control_data->head;
    // Loop over the hardware scripts, get their arguments, fork process, call execvp to spawn child
    // python script process
    for (int pid_counter = 0; hw_script_ptr != NULL; pid_counter++){
        char *script_args[7];
        script_args[0] = "python3";
        script_args[1] = strdup(hw_script_ptr->filePath);
        script_args[2] = strdup(control_data->controlIP);
        script_args[3] = strdup(hw_script_ptr->listenerPort);
        script_args[4] = strdup(hw_script_ptr->deviceIP);
        script_args[5] = strdup(hw_script_ptr->devicePort);
        script_args[6] = NULL;
        hw_script_ptr = hw_script_ptr->next;
        child_processes[pid_counter] = fork();
        // If we're now in a child process, call execvp and execute the current python script
        if (child_processes[pid_counter] == 0) {
            //g_print("Successful fork, calling script %s\n", hw_script_ptr->name);
            execvp(script_args[0], script_args);
        }
        // fork() failed, kill the program
        else if (child_processes[pid_counter] == -1) {
            timestamp_prefix_err();
            g_printerr("Couldn't fork process for script %s", script_args[1]);
            for (pid_t i = 0; i < pid_counter; i++) {
                kill(child_processes[i], SIGINT);
            }
            free(child_processes);
            err_code = EXIT_FAILURE;
            goto cleanup_udp_listener;
        }
        // free all the strdup'd strings
        for (int free_counter = 1; free_counter < 6; free_counter++){
            free(script_args[free_counter]);
        }
    }

    // ---------- Run the main event loop ---------------
    // runs until it doesn't
    g_main_loop_run(loop);


    // ---------- Cleanup ----------
    

	cleanup_hardware_scripts:
	    // This might not be the correct way to do this, but it should work???
    	for (int kill_counter = 0; kill_counter < number_of_hardware_scripts; kill_counter++){
        	kill(child_processes[kill_counter], SIGINT);
    	}
    	// Free pid_t array
    	free(child_processes);
    cleanup_udp_listener:
        stopUDPListener(udp_listener);
    cleanup_status_broadcast:
        g_source_remove(status_broadcast_id);
    cleanup_rtsp_server:
        g_source_remove(rtsp_server_id);
        g_object_unref(rtsp_server);
    cleanup_csv_pipeline:
		gst_element_set_state(pipeline_data.csv_pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline_data.csv_pipeline);
	cleanup_pipeline_string:
		g_free(pipeline_string);
    cleanup_config:
        free_Metadata(metadata);
        free_ControlData(control_data);
    cleanup_sensor_setup_info:
        freeSensorSetupInfo(sensor_setup_info);
    cleanup_loop:
        g_main_loop_unref(loop);
    if (err_code == EXIT_SUCCESS) {
        timestamp_prefix_log();
        g_print("Camera program exited sucessfully.\n");
    } else {
        timestamp_prefix_err();
        g_print("Camera program exited with error.\n");
    }
    return err_code;
}
