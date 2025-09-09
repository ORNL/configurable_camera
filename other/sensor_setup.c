#include <glib.h>
#include <gst/gst.h>

#include "sensor.h"

int main(int argc, char* argv[]){

    gst_init(&argc, &argv);

    gchar* working_directory;

    if (argc == 2) {
        working_directory = argv[1];
    } else {
        working_directory = ".";
    }
        
    gchar* sensor_path = g_strdup_printf("%s/%s", working_directory, DEFAULT_SENSOR_JSON);

    g_print("Writing sensor setup info to %s\n", sensor_path);
    int ret = writeSensorSetupJSON(sensor_path);

    g_free(sensor_path);

    if (ret != 0) {
        g_printerr("Could not write sensor setup file.\n");
        return -1;
    }
    return 0;
}
