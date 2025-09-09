#ifndef SENSOR_H
#define SENSOR_H

#include <time.h>

#include <gst/gst.h>

#define METADATA_CSV_HEADER "Time (s), Gain (dB), Auto Gain Setting, Exposure (us), Auto Exposure Setting, Black Level, Auto Target Brightness, Frame Rate, Sensor Temperature, Temperature State\n"
#define DEFAULT_SENSOR_JSON "sensor.json"

enum {TEMPERATURE_OK, TEMPERATURE_CRITICAL, TEMPERATURE_ERROR, TEMPERATURE_NONE}; // Sensor temperature states
enum {AUTO_OFF, AUTO_ONCE, AUTO_CONTINUOUS}; // auto gain and exposure settings
enum {BINNING_SUM, BINNING_AVERAGE, BINNING_NONE}; // pixel binning mode
enum {SHUTTER_GLOBAL=2}; // shutter mode

typedef struct SensorSetupInfo {
    gchar *name; //DeviceModelName
    gchar *serial_number; //DeviceSerialNumber
    gchar *caps_type; // formatted name for gstreamer caps linking, e.g. video/x-bayer
    gchar *pixel_color_filter; // either GRAY8 or a bayer format (e.g. bggr, gbgr, etc.)
    int is_color;
    int width;
    int height;
    int gain_auto;
    double gain;
    int exposure_auto;
    double exposure_time;
    // only filled out if loaded from sensor.json
    gchar *full_caps_string;
} SensorSetupInfo;

// TODO: might as well combine these two structs at this point

typedef struct SensorStaticInfo {
    SensorSetupInfo *sensor_setup_info;
    int pixel_size;
    int pixel_dynamic_range_min;
    int pixel_dynamic_range_max;
    int binning_horizontal_mode;
    int binning_vertical_mode;
    int binning_horizontal;
    int binning_vertical;
    int shutter_mode;
    double gamma;
} SensorStaticInfo;

typedef struct SensorDynamicInfo {
    gchar *name; //DeviceModelName

    time_t timestamp;
    long addl_nanoseconds;
    double temperature;
    int temperature_state;
    int gain_auto;
    double gain;
    int exposure_auto;
    double exposure_time;
    double black_level;
    double auto_target_brightness;
    double frame_rate;
} SensorDynamicInfo; 

SensorSetupInfo *getSensorSetupInfo(GstElement *pylonsrc);

void freeSensorSetupInfo(SensorSetupInfo *sensor_setup_info);

SensorStaticInfo *getSensorStaticInfo(GstElement *pylonsrc);

void freeSensorStaticInfo(SensorStaticInfo *sensor_static_info);

SensorDynamicInfo *getSensorDynamicInfo(GstElement *pylonsrc);

void freeSensorDynamicInfo(SensorDynamicInfo *sensor_dynamic_info);

// record sensor metadata
char *formatSensorDynamicInfoCSV(SensorDynamicInfo *si);

char *sensorSetupInfoJSON(SensorSetupInfo *ssi);

int writeSensorSetupJSON(const char* sensor_setup_json_path);

SensorSetupInfo *loadSensorSetupInfo(const char* sensor_setup_json_path);

char *sensorSetupInfoJSON(SensorSetupInfo *sensor_setup_info);

char *sensorStaticInfoJSON(SensorStaticInfo *ssi);

// setters

void setSensorGain(GstElement *pylonsrc, double dB);

void setSensorExposureTime(GstElement *pylonsrc, double microseconds);

#endif
