#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gst/gst.h>

#include "sensor.h"


int main(int argc, char *argv[]) {

    GstElement *source;

    gst_init(&argc, &argv);

    source = gst_element_factory_make("pylonsrc", "source");

    SensorSetupInfo *ssi = getSensorSetupInfo(source);
    if (ssi == NULL) {
        g_error("Failed to get sensor setup info\n");
        return -1;
    }
    char *sensor_setup_json = sensorSetupInfoJSON(ssi);
    g_print("%s\n", sensor_setup_json);
    freeSensorSetupInfo(ssi);
    free(sensor_setup_json);

    setSensorGain(source, 4.0);

    SensorDynamicInfo *sensor_info;
    char *csv_line;
    
    for (int i = 0; i < 5; i++) {
        sensor_info = getSensorDynamicInfo(source);

        csv_line = formatSensorDynamicInfoCSV(sensor_info);

        g_print("%s", csv_line);

        freeSensorDynamicInfo(sensor_info);
        free(csv_line);
        csv_line = NULL;
        sleep(1);
    }

    gst_object_unref(source);

    return 0;
}
