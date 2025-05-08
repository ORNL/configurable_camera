#include <gst/gst.h>

#include "sensor.h"

int main(int argc, char* argv[]){

    gst_init(&argc, &argv);

    int ret = writeSensorSetupJSON();

    if (ret != 0) {
        g_printerr("Could not write sensor setup file.\n");
        return -1;
    }
    g_print("Wrote sensor setup info to %s\n", SENSOR_SETUP_JSON);
    return 0;
}
