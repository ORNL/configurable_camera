#ifndef CONFIG_H
#define CONFIG_H

#include "cJSON.h"

#define DEFAULT_CONFIG_FILEPATH "~/configurable_code/config.json"
#define DEFAULT_PIPELINE_FILEPATH "~/configurable_code/pipeline.txt"
#define IP_STR_MAX_LEN 16
#define DATE_LEN 8
#define BROADCAST_ADDRESS "255.255.255.255"

typedef struct _ControlData {

  // TODO: Add recording path
  int recordRaw;
  char *rawStorage;
  char *controlIP;
  char *broadcastAddress;
  int statusBroadcastPort;
  int controlPort;
  int rawTimeLimit;
  char *RTSPaddress;
  char *location;
  char *hostname;

  /*Pointer to first hScript struct*/
  struct _HardwareScripts *head;

  // keep the json in memory for the hardware script capabilities
  // free it in free_ControlData
  cJSON *json;

} ControlData;

typedef struct _HardwareScripts {

  char *name;
  char *filePath;
  char *listenerPort;
  char *deviceIP;
  char *devicePort;
  cJSON *capabilities;

  /*Pointer to next hardware script section*/
  struct _HardwareScripts *next;

} HardwareScripts;

/*Struct to store the camera config data*/
typedef struct _Metadata {
  char *name;
  char *metadata;
  char *location;
  int range;
  int yaw;
  int elevated;
  char *lensName;
  int focalLength;
  char *collectionID;

} Metadata;

// typedef struct _ConfigData ConfigData;

//     ConfigData struct implementation fields

//     Metadata* metadata;
//     ControlData* controlData;

// TODO: use getenv to retrieve environment variable set in Makefile "CONFIG_FILEPATH"
char *getConfigFilepath(void);

/*Functions to free data from the values in the structs*/
void free_HardwareScripts(HardwareScripts *data);
void free_ControlData(ControlData *data);
void free_Metadata(Metadata *data);

int readConfigFile(char *config_filepath, 
    ControlData **control_data_ptr, 
    Metadata **metadata_ptr);

void printConfigSummary(ControlData *control_data, Metadata *metadata,
                        HardwareScripts *hardware_scripts);

int format_recording_directory(ControlData *control_data, char **filepath_ptr);

int get_addresses(ControlData *control_data);

int copy_file(const char *from, const char *to);

char *gst_pipeline_txt_gen(const char *filepath, int rtsp_only);

int replace_with_address(char** interface_name);

#endif
