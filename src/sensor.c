#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <gst/gst.h>

#include "log.h"
#include "cJSON.h"
#include "sensor.h"

#define METADATA_CSV_LINE_BUFFER 2048
#define FILE_BUFFER_SIZE 1024

#define FRAMERATE 30


SensorSetupInfo *getSensorSetupInfo(GstElement *pylonsrc) {

    SensorSetupInfo *sensor_setup_info = (SensorSetupInfo*) malloc(sizeof(SensorSetupInfo));
    memset(sensor_setup_info, 0, sizeof(SensorSetupInfo));
    
    sensor_setup_info->pixel_color_filter = NULL;
    int pixel_color_filter_int;

    sensor_setup_info->name = NULL;

    gst_child_proxy_get(GST_CHILD_PROXY(pylonsrc),
        "cam::DeviceModelName", &(sensor_setup_info->name),
        NULL);

    if (sensor_setup_info->name == NULL) {
        timestamp_prefix_err();
        g_printerr("Couldn't get name of sensor, sensor likely isn't connected\n");
        goto error;
    }

    gst_child_proxy_get(GST_CHILD_PROXY(pylonsrc),
        "cam::DeviceSerialNumber", &(sensor_setup_info->serial_number),
        NULL);

    if (!strcmp(sensor_setup_info->name, "acA4112-30um")){
        sensor_setup_info->width = 3840;
        sensor_setup_info->height = 2160;
    }
    else if (!strcmp(sensor_setup_info->name, "acA2040-90um")){
        sensor_setup_info->width = 2048;
        sensor_setup_info->height = 2048;
    }
    else if (!strcmp(sensor_setup_info->name, "acA2040-90uc")){
        sensor_setup_info->width = 2040;
        sensor_setup_info->height = 2046;
    }
    else if (!strcmp(sensor_setup_info->name, "acA2040-120um")){
        sensor_setup_info->width = 2048;
        sensor_setup_info->height = 1536;
    }
    else if (!strcmp(sensor_setup_info->name, "acA2040-120uc")){
        sensor_setup_info->width = 2048;
        sensor_setup_info->height = 1536;
    }
    else if (!strcmp(sensor_setup_info->name, "acA4112-30uc")){
        sensor_setup_info->width = 3840;
        sensor_setup_info->height = 2160;
    }
    else if (!strcmp(sensor_setup_info->name, "a2A3840-45ucPRO")){
        sensor_setup_info->width = 3840;
        sensor_setup_info->height = 2160;
    }
    else if (!strcmp(sensor_setup_info->name, "a2A5328-15ucPRO")){
        sensor_setup_info->width = 5328;
        sensor_setup_info->height = 4608;
    }
    else if (!strcmp(sensor_setup_info->name, "acA4096-30uc")){
        sensor_setup_info->width = 4096;
        sensor_setup_info->height = 2168;
    }
    else if (!strcmp(sensor_setup_info->name, "acA4096-30um")){
        sensor_setup_info->width = 4096;
        sensor_setup_info->height = 2168;
    }
    else if (!strcmp(sensor_setup_info->name, "acA1920-155um")){
        sensor_setup_info->width = 1920;
        sensor_setup_info->height = 1200;
    }
    else if (!strcmp(sensor_setup_info->name, "acA1920-150uc")){
        sensor_setup_info->width = 1920;
        sensor_setup_info->height = 1200;
    }
    // TODO: Is this the behavior we want if we have a sensor we don't recognize?
    else{
        g_warning("Sensor %s does not have preset resolution settings. Defaulting to 1080p.\n", sensor_setup_info->name);
        sensor_setup_info->width = 1920;
        sensor_setup_info->height = 1080;
    }

    gst_child_proxy_get(GST_CHILD_PROXY(pylonsrc),
        "cam::PixelColorFilter", &(pixel_color_filter_int),
        NULL);

    sensor_setup_info->is_color = TRUE;

    switch (pixel_color_filter_int) {
        case 0:
            sensor_setup_info->is_color = FALSE;
            sensor_setup_info->pixel_color_filter = strdup("GRAY8");
            sensor_setup_info->caps_type = strdup("video/x-raw");
            break;
        case 1:
            sensor_setup_info->pixel_color_filter = strdup("rggb");
            sensor_setup_info->caps_type = strdup("video/x-bayer");
            break;
        case 2:
            sensor_setup_info->pixel_color_filter = strdup("gbrg");
            sensor_setup_info->caps_type = strdup("video/x-bayer");
            break;
        case 3:
            sensor_setup_info->pixel_color_filter = strdup("grbg");
            sensor_setup_info->caps_type = strdup("video/x-bayer");
            break;
        case 4:
            sensor_setup_info->pixel_color_filter = strdup("bggr");
            sensor_setup_info->caps_type = strdup("video/x-bayer");
            break;
        default:
            timestamp_prefix_err();
            g_printerr("Unrecognized color filter format.\n");
            goto error;
    }

    gst_child_proxy_get(GST_CHILD_PROXY(pylonsrc),
        "cam::Gain", &(sensor_setup_info->gain), NULL);
    gst_child_proxy_get(GST_CHILD_PROXY(pylonsrc),
        "cam::GainAuto", &(sensor_setup_info->gain_auto), NULL);
    gst_child_proxy_get(GST_CHILD_PROXY(pylonsrc),
        "cam::ExposureTime", &(sensor_setup_info->exposure_time), NULL);
    gst_child_proxy_get(GST_CHILD_PROXY(pylonsrc),
        "cam::ExposureAuto", &(sensor_setup_info->exposure_auto), NULL);

    return sensor_setup_info;

    error:
        timestamp_prefix_err();
        g_printerr("Failed to get sensor setup info.\n");
        free(sensor_setup_info);
        return NULL;
}

void freeSensorSetupInfo(SensorSetupInfo *sensor_setup_info) {
    if (sensor_setup_info != NULL) {
        free(sensor_setup_info->pixel_color_filter);
        sensor_setup_info->pixel_color_filter = NULL;
        free(sensor_setup_info->caps_type);
        sensor_setup_info->caps_type = NULL;

        // if full_caps_string has been formatted, the struct has been loaded
        // from a file using loadSensorSetupInfo. name and serial number also need to be freed
        if (sensor_setup_info->full_caps_string != NULL) {
            free(sensor_setup_info->full_caps_string);
            sensor_setup_info->full_caps_string = NULL;
            free(sensor_setup_info->name);
            sensor_setup_info->name = NULL;
            free(sensor_setup_info->serial_number);
            sensor_setup_info->serial_number = NULL;
        }
        free(sensor_setup_info);
        sensor_setup_info = NULL;
    }
}

SensorStaticInfo *getSensorStaticInfo(GstElement *pylonsrc) {
    // TODO: Error handling
    SensorStaticInfo *ssi = (SensorStaticInfo *)malloc(sizeof(SensorStaticInfo));
    memset(ssi, 0, sizeof(SensorStaticInfo));
    // ----------------- Get sensor attributes ---------------------

    ssi->sensor_setup_info = getSensorSetupInfo(pylonsrc);
    if (ssi->sensor_setup_info == NULL) {
        goto error;
    }
    
    gst_child_proxy_get(GST_CHILD_PROXY(pylonsrc),
        "cam::PixelSize", &(ssi->pixel_size),
        NULL);
    gst_child_proxy_get(GST_CHILD_PROXY(pylonsrc),
        "cam::PixelDynamicRangeMin", &(ssi->pixel_dynamic_range_min),
        "cam::PixelDynamicRangeMax", &(ssi->pixel_dynamic_range_max),
        NULL);
    gst_child_proxy_get(GST_CHILD_PROXY(pylonsrc),
        "cam::ShutterMode", &(ssi->shutter_mode),
        NULL);
    gst_child_proxy_get(GST_CHILD_PROXY(pylonsrc),
        "cam::Gamma", &(ssi->gamma),
        NULL);

    ssi->binning_horizontal_mode = -1;
    ssi->binning_vertical_mode = -1;
    ssi->binning_horizontal = -1;
    ssi->binning_vertical = -1;
    
    gst_child_proxy_get(GST_CHILD_PROXY(pylonsrc),
        "cam::BinningHorizontalMode", &(ssi->binning_horizontal_mode),
        "cam::BinningVerticalMode", &(ssi->binning_vertical_mode),
        "cam::BinningHorizontal", &(ssi->binning_horizontal),
        "cam::BinningVertical", &(ssi->binning_vertical),
        NULL);

    if (ssi->binning_horizontal == -1) {
        timestamp_prefix_log();
        g_print("Sensor %s has no pixel binning settings.\n", ssi->sensor_setup_info->name);
    }

    return ssi;

    error:
        timestamp_prefix_err();
        g_printerr("Failed to get sensor static info.\n");
        freeSensorStaticInfo(ssi);
        return NULL;
}

void freeSensorStaticInfo(SensorStaticInfo *ssi) {
    if (ssi != NULL) {
        freeSensorSetupInfo(ssi->sensor_setup_info);
        ssi->sensor_setup_info = NULL;
        free(ssi);
        ssi = NULL;
    }
}

SensorDynamicInfo *getSensorDynamicInfo(GstElement *pylonsrc) {

    SensorDynamicInfo *sensor_info = malloc(sizeof(SensorDynamicInfo));
    if (sensor_info == NULL) {
        timestamp_prefix_err();
        g_printerr("Failed to allocate SensorDynamicInfo.\n");
        return NULL;
    }
    memset(sensor_info, 0, sizeof(SensorDynamicInfo));

    double gain;
    gst_child_proxy_get(GST_CHILD_PROXY(pylonsrc),
        "cam::Gain", &gain, NULL);

    sensor_info->gain = gain;
    
    gst_child_proxy_get(GST_CHILD_PROXY(pylonsrc),
        "cam::DeviceModelName", &(sensor_info->name),
        NULL);

    if (sensor_info->name == NULL) {
        timestamp_prefix_err();
        g_printerr("Couldn't get name of sensor, sensor likely isn't connected\n");
        return NULL;
    }

    gst_child_proxy_get(GST_CHILD_PROXY(pylonsrc),
        "cam::GainAuto", &(sensor_info->gain_auto), NULL);
    gst_child_proxy_get(GST_CHILD_PROXY(pylonsrc),
        "cam::ExposureTime", &(sensor_info->exposure_time), NULL);
    gst_child_proxy_get(GST_CHILD_PROXY(pylonsrc),
        "cam::ExposureAuto", &(sensor_info->exposure_auto), NULL);
    gst_child_proxy_get(GST_CHILD_PROXY(pylonsrc),
        "cam::BlackLevel", &(sensor_info->black_level), NULL);
    gst_child_proxy_get(GST_CHILD_PROXY(pylonsrc),
        "cam::AutoTargetBrightness", &(sensor_info->auto_target_brightness), NULL);
    gst_child_proxy_get(GST_CHILD_PROXY(pylonsrc),
        "cam::ResultingFrameRate", &(sensor_info->frame_rate), NULL);

    static int has_temp = -1;
    if (has_temp == -1) {
        sensor_info->temperature = 0;
        sensor_info->temperature_state = TEMPERATURE_NONE;
        gst_child_proxy_get(GST_CHILD_PROXY(pylonsrc),
            "cam::DeviceTemperature", &(sensor_info->temperature), NULL);
        gst_child_proxy_get(GST_CHILD_PROXY(pylonsrc),
            "cam::TemperatureState", &(sensor_info->temperature_state), NULL);

        if (sensor_info->temperature == 0 || sensor_info->temperature_state == TEMPERATURE_NONE) {
            has_temp = 0;
            // set it to a valid value, never touch it again.
            sensor_info->temperature_state = TEMPERATURE_OK;
        } else {
            has_temp = 1;
        }
    } else if (has_temp == 1) {
        gst_child_proxy_get(GST_CHILD_PROXY(pylonsrc),
            "cam::DeviceTemperature", &(sensor_info->temperature), NULL);
        gst_child_proxy_get(GST_CHILD_PROXY(pylonsrc),
            "cam::TemperatureState", &(sensor_info->temperature_state), NULL);
    }

    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        timestamp_prefix_err();
        g_printerr("Failed to get system time.\n");
        return NULL;
    }

    sensor_info->timestamp = ts.tv_sec;
    sensor_info->addl_nanoseconds = ts.tv_nsec;


    return sensor_info;
}

void freeSensorDynamicInfo(SensorDynamicInfo *sensor_dynamic_info) {
    free(sensor_dynamic_info);
    sensor_dynamic_info = NULL;
}

char *formatSensorDynamicInfoCSV(SensorDynamicInfo *si) {
    /*
    Get all the dynamic sensor info, format it into a line of csv
    so it can be included in the metadata
    */
    char buffer[METADATA_CSV_LINE_BUFFER];

    char * gain_auto;
    if (si->gain_auto == AUTO_OFF) {
        gain_auto = strdup("Off");
    } else if (si->gain_auto == AUTO_ONCE) {
        gain_auto = strdup("Once");
    } else if (si->gain_auto == AUTO_CONTINUOUS) {
        gain_auto = strdup("Continuous");
    } else {
        gain_auto = strdup("Bad Value");
        return NULL;
    }

    char *exposure_auto;
    if (si->exposure_auto == AUTO_OFF) {
        exposure_auto = strdup("Off");
    } else if (si->exposure_auto == AUTO_ONCE) {
        exposure_auto = strdup("Once");
    } else if (si->exposure_auto == AUTO_CONTINUOUS) {
        exposure_auto = strdup("Continuous");
    } else {
        exposure_auto = strdup("Bad Value");
        return NULL;
    }

    char *temperature_state;
    if (si->temperature_state == TEMPERATURE_OK) {
        temperature_state = strdup("OK");
    } else if (si->temperature_state == TEMPERATURE_CRITICAL) {
        temperature_state = strdup("Critical");
    } else if (si->temperature_state == TEMPERATURE_ERROR) {
        temperature_state = strdup("Error");
    } else {
        timestamp_prefix_err();
        g_printerr("Bad temperature state value.\n");
		temperature_state = strdup("None");
        //return NULL;
    }
    
    // timestamp, gain (dB), auto gain setting, exposure (ms), auto exposure setting, black level, auto target brightness, resulting frame rate, device temperature, device temperature state
    char *format_string = "%d.%d,%f,%s,%f,%s,%f,%f,%f,%f,%s\n";
    snprintf(buffer, METADATA_CSV_LINE_BUFFER, format_string,
        si->timestamp,
        si->addl_nanoseconds,
        si->gain,
        gain_auto,
        si->exposure_time,
        exposure_auto,
        si->black_level,
        si->auto_target_brightness,
        si->frame_rate,
        si->temperature,
        temperature_state
    );

    free(gain_auto);
    free(exposure_auto);
    free(temperature_state);

    char *csv_line = strdup(buffer);
    return csv_line;
}


// --------- Sensor Metadata Formatting -----------

static cJSON *sensorSetupInfoCJSON(SensorSetupInfo *ssi) {
    /*
    Format a cJSON object from the SensorSetupInfo
    must be freed
    */

    cJSON *root;

    root = cJSON_CreateObject();
    
    cJSON_AddItemToObject(root, "Model", cJSON_CreateString(ssi->name));
    cJSON_AddItemToObject(root, "Serial Number", cJSON_CreateString(ssi->serial_number));
    cJSON_AddItemToObject(root, "Width", cJSON_CreateNumber(ssi->width));
    cJSON_AddItemToObject(root, "Height", cJSON_CreateNumber(ssi->height));
    cJSON_AddItemToObject(root, "Caps Type", cJSON_CreateString(ssi->caps_type));
    cJSON_AddItemToObject(root, "Color Filter", cJSON_CreateString(ssi->pixel_color_filter));

    // casting to int to avoid weird float reformatting issues
    cJSON_AddItemToObject(root, "Exposure Time", cJSON_CreateNumber((int)ssi->exposure_time));
    cJSON_AddItemToObject(root, "Gain", cJSON_CreateNumber((int)ssi->gain));
    
    char *auto_setting;
    int num_settings = 2;
    char *settings[] = {"Auto Exposure", "Auto Gain"};
    int autos[] = {ssi->exposure_auto, ssi->gain_auto};
    for (int i = 0; i < num_settings; i++) {
        if (autos[i] == AUTO_OFF) {
            auto_setting = "AUTO_OFF";
        } else if (autos[i] == AUTO_ONCE) {
            auto_setting = "AUTO_ONCE";
        } else if (autos[i] == AUTO_CONTINUOUS) {
            auto_setting = "AUTO_CONTINUOUS";
        } else {
            auto_setting = "ERROR";
        }
        cJSON_AddItemToObject(root, settings[i] ,cJSON_CreateString(auto_setting));
    }
    cJSON_AddItemToObject(root, "NVMM", cJSON_CreateBool(FALSE));

    return root;
}

char *sensorSetupInfoJSON(SensorSetupInfo *ssi) {
    /*
    Stringifies the cJSON object
    */
    cJSON *root = sensorSetupInfoCJSON(ssi);
    char *json_string = cJSON_Print(root);
    cJSON_Delete(root);
    return json_string;
}

static int writeToSensorSetupJSON(const char* sensor_setup_json_path, const char* json_string) {
    FILE *sensor_metadata_file;
    sensor_metadata_file = fopen(sensor_setup_json_path, "w");
    if (sensor_metadata_file == NULL) {
        timestamp_prefix_err();
        g_printerr("Unable to open sensor.json for writing.\n");
        fclose(sensor_metadata_file);
        return -1;
    }
    int rc = fputs(json_string, sensor_metadata_file);
    fclose(sensor_metadata_file);
    if (rc == EOF) {
        timestamp_prefix_err();
        g_printerr("Failed to write to sensor.json\n");
        return -1;
    }
    return 0;
}

int writeSensorSetupJSON(const char *sensor_setup_json_path) {
    GstElement *pylonsrc;
    pylonsrc = gst_element_factory_make("pylonsrc", "sensor_setup");
    if (!pylonsrc) {
        g_warning("Could not create pylonsrc object.\n");
        return -1;
    }
    SensorSetupInfo *ssi = getSensorSetupInfo(pylonsrc);
    gst_object_unref(pylonsrc);
    if (ssi == NULL) {
        return -1;
    }

    char *sensor_setup_json = sensorSetupInfoJSON(ssi);

    int ret = writeToSensorSetupJSON(sensor_setup_json_path, sensor_setup_json);

    free(sensor_setup_json);
    freeSensorSetupInfo(ssi);
    return ret;
}

static cJSON *parseSensorSetupJSON(const char *sensor_setup_json_path) {
    /*
    Get parsed JSON from the SENSOR_SETUP_JSON file
    */
    char file_buffer[FILE_BUFFER_SIZE];
    memset(file_buffer, '\0', FILE_BUFFER_SIZE);

    FILE *fp = fopen(sensor_setup_json_path, "r");
    if (fp == NULL) {
        timestamp_prefix_err();
        g_printerr("Unable to open sensor.json for reading.\n");
        return NULL;
    }
    // read the file contents into a string
    int read = fread(file_buffer, sizeof(char), FILE_BUFFER_SIZE, fp);
    fclose(fp);

    if (read <= 0) {
        timestamp_prefix_err();
        g_printerr("Failed to read from sensor file.\n");
        return NULL;
    }

    // parse the JSON data
    cJSON *json = cJSON_Parse(file_buffer);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            timestamp_prefix_err(); 
            g_printerr("Failed to parse sensor setup info: %s\n", error_ptr);
        }
        return NULL;
    }

    return json;
}

SensorSetupInfo *loadSensorSetupInfo(const char* sensor_setup_json_path) {
    /*
    Load sensor information from a JSON file written by
    writeSensorSetupJSON.
    Returns a SensorSetupInfo struct with a fully formatted caps string
    for the gstreamer pipeline
    */

    cJSON *json = parseSensorSetupJSON(sensor_setup_json_path);

    SensorSetupInfo *ssi = (SensorSetupInfo *)malloc(sizeof(SensorSetupInfo));
    memset(ssi, 0, sizeof(SensorSetupInfo));
    int use_nvmm;

    for (cJSON *ob = json->child; ob != NULL; ob = ob->next) {
        if (strcmp(ob->string, "Model") == 0) {
            ssi->name = strdup(ob->valuestring);
        } else if (strcmp(ob->string, "Serial Number") == 0) {
            ssi->serial_number = strdup(ob->valuestring);
        } else if (strcmp(ob->string, "Caps Type") == 0) {
            ssi->caps_type = strdup(ob->valuestring);
            if (strcmp(ssi->caps_type, "video/x-bayer")) {
                ssi->is_color = FALSE;
            } else {
                ssi->is_color = TRUE;
            }
        } else if (strcmp(ob->string, "Color Filter") == 0) {
            ssi->pixel_color_filter = strdup(ob->valuestring);
        } else if (strcmp(ob->string, "Width") == 0) {
            ssi->width = ob->valueint;
        } else if (strcmp(ob->string, "Height") == 0) {
            ssi->height = ob->valueint;
        } else if (strcmp(ob->string, "Exposure Time") == 0) {
            ssi->exposure_time = (double)ob->valueint;
        } else if (strcmp(ob->string, "Gain") == 0) {
            ssi->gain = (double)ob->valueint;
        } else if (strcmp(ob->string, "Auto Exposure") == 0) {
            if (strcmp(ob->valuestring, "AUTO_OFF") == 0) {
                ssi->exposure_auto = AUTO_OFF;
            } else if (strcmp(ob->valuestring, "AUTO_ONCE") == 0) {
                ssi->exposure_auto = AUTO_ONCE;
            } else if (strcmp(ob->valuestring, "AUTO_CONTINUOUS") == 0) {
                ssi->exposure_auto = AUTO_CONTINUOUS;
            } else {
                g_print("Warning: Invalid Auto Exposure value in sensor.json. Setting to OFF.\n");
                ssi->exposure_auto = AUTO_OFF;
            }
        } else if (strcmp(ob->string, "Auto Gain") == 0) {
            if (strcmp(ob->valuestring, "AUTO_OFF") == 0) {
                ssi->gain_auto = AUTO_OFF;
            } else if (strcmp(ob->valuestring, "AUTO_ONCE") == 0) {
                ssi->gain_auto = AUTO_ONCE;
            } else if (strcmp(ob->valuestring, "AUTO_CONTINUOUS") == 0) {
                ssi->gain_auto = AUTO_CONTINUOUS;
            } else {
                g_print("Warning: Invalid Auto Gain value in sensor.json. Setting to OFF.\n");
                ssi->gain_auto = AUTO_OFF;
            }
        } else if (strcmp(ob->string, "NVMM") == 0) {
            use_nvmm = ob->valueint;
        }
    }
    free(json);

    if (ssi->name == NULL ||
            ssi->serial_number == NULL ||
            ssi->caps_type == NULL || 
            ssi->pixel_color_filter == NULL || 
            ssi->width == 0 || 
            ssi->height == 0) {
        printf("Failed to read sensor info.\n");
        goto error;
    }
    char *nvmm_string;
    if (use_nvmm) {
        nvmm_string = strdup("(memory:NVMM)");
    } else {
        nvmm_string = strdup("");
    }

    char caps[1024];
    int written = snprintf(caps, 1024, "%s%s,format=%s,width=%d,height=%d,framerate=%d/1", 
        ssi->caps_type, nvmm_string, ssi->pixel_color_filter, ssi->width, ssi->height, FRAMERATE);
    if (written == 0 || written < 0 || written >= 1024) {
        printf("Error writing caps string; overflow or failed write.\n");
        goto snprintferror;
    }
    free(nvmm_string);

    ssi->full_caps_string = strdup(caps);
    return ssi;

    snprintferror:
        free(nvmm_string);
    error:
        freeSensorSetupInfo(ssi);
        return NULL;
}

char *sensorStaticInfoJSON(SensorStaticInfo *ssi) {
    
    cJSON *root = sensorSetupInfoCJSON(ssi->sensor_setup_info);

    cJSON_AddItemToObject(root, "Pixel Bit Depth", cJSON_CreateNumber(ssi->pixel_size));
    cJSON_AddItemToObject(root, "Gamma", cJSON_CreateNumber(ssi->gamma));

        
    if (ssi->binning_horizontal == -1) {
        cJSON_AddItemToObject(root, "Horizontal Binning", cJSON_CreateNumber(1));
    } else {
        cJSON_AddItemToObject(root, "Horizontal Binning", cJSON_CreateNumber(ssi->binning_horizontal));
    }

    if (ssi->binning_horizontal_mode == BINNING_AVERAGE) {
        cJSON_AddItemToObject(root, "Horizontal Binning Mode", cJSON_CreateString("Average"));
    } else if (ssi->binning_horizontal_mode == BINNING_SUM) {
        cJSON_AddItemToObject(root, "Horizontal Binning Mode", cJSON_CreateString("Sum"));
    } else if (ssi->binning_horizontal_mode == -1) {
        cJSON_AddItemToObject(root, "Horizontal Binning Mode", cJSON_CreateString("None"));
    } else {
        timestamp_prefix_err();
        g_printerr("Bad horizontal binning mode value: %d\n", ssi->binning_horizontal_mode);
        cJSON_Delete(root);
        return NULL;
    }

    if (ssi->binning_vertical == -1) {
        cJSON_AddItemToObject(root, "Vertical Binning", cJSON_CreateNumber(1));
    } else {
        cJSON_AddItemToObject(root, "Vertical Binning", cJSON_CreateNumber(ssi->binning_vertical));
    }
    
    if (ssi->binning_vertical_mode == BINNING_AVERAGE) {
        cJSON_AddItemToObject(root, "Vertical Binning Mode", cJSON_CreateString("Average"));
    } else if (ssi->binning_vertical_mode == BINNING_SUM) {
        cJSON_AddItemToObject(root, "Vertical Binning Mode", cJSON_CreateString("Sum"));
    } else if (ssi->binning_vertical_mode == -1) {
        cJSON_AddItemToObject(root, "Vertical Binning Mode", cJSON_CreateString("None"));
    } else {
        timestamp_prefix_err();
        g_printerr("Bad vertical binning mode value: %d\n", ssi->binning_vertical_mode);
        cJSON_Delete(root);
        return NULL;
    }
    if (ssi->shutter_mode == SHUTTER_GLOBAL) {
        cJSON_AddItemToObject(root, "Shutter Mode", cJSON_CreateString("Global"));
    } else {
        cJSON_AddItemToObject(root, "Shutter Mode", cJSON_CreateString("Unknown"));
    }

    char *json_string = cJSON_Print(root);
    cJSON_Delete(root);
    return json_string;
}

// setters

void setSensorGain(GstElement *pylonsrc, double dB) {
    gst_child_proxy_set(GST_CHILD_PROXY(pylonsrc),
        "cam::Gain", dB,
        NULL);
}

void setSensorExposureTime(GstElement *pylonsrc, double microseconds) {
    gst_child_proxy_set(GST_CHILD_PROXY(pylonsrc),
        "cam::ExposureTime", microseconds,
        NULL);
}

void setSensorAutoGain(GstElement *pylonsrc, int setting) {
    gst_child_proxy_set (GST_CHILD_PROXY(pylonsrc), 
        "cam::GainAuto", setting, 
        NULL);
}

void setSensorAutoExposure(GstElement *pylonsrc, int setting) {
    gst_child_proxy_set (GST_CHILD_PROXY(pylonsrc), 
        "cam::ExposureAuto", setting, 
        NULL);
}
