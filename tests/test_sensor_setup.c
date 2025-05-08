#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gst/gst.h>

#include "sensor.h"


int main(int argc, char *argv[]) {

    gst_init(&argc, &argv);

    if (sensorSetupJSON(SENSOR_SETUP_JSON) != 0) {
        g_error("couldn't write sensor setup file\n");
        return -1;
    }

    char *caps_string = loadSensorCaps(SENSOR_SETUP_JSON);
    g_print("%s\n", caps_string);

}
