#include <stdio.h>

#include <gst/gst.h>
#include <gio/gio.h>

#include "cJSON.h"

#include "config.h"
#include "statusbroadcaster.h"
#include "sensor.h"

int main() {

    char *config_filepath = "~/configurable_code/tests/config.json";

    StatusBroadcast status_broadcast;

    SensorInfo sensor_info;
    status_broadcast.sensor_info = &sensor_info;

    status_broadcast.sensor_info->name = strdup("test_sensor_name");
    status_broadcast.sensor_info->temperature = 50;
    status_broadcast.sensor_info->temperature_state = TEMPERATURE_OK;

    cJSON *json = readConfigFile(config_filepath, &(status_broadcast.control_data), &(status_broadcast.metadata));


    GMainLoop *loop = g_main_loop_new(NULL, FALSE);

    setup_socket(&status_broadcast);
    g_print("connected\n");

    g_timeout_add_seconds(TIMEOUT_TIME, status_broadcaster, (gpointer) &status_broadcast);
    g_main_loop_run(loop);

    g_main_loop_unref(loop);
    g_object_unref(status_broadcast.socket);
    metadata_Delete(status_broadcast.metadata);
    controldata_Delete(status_broadcast.control_data);
    cJSON_Delete(json);
    free(status_broadcast.sensor_info->name);
}
