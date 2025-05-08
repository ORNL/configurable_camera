#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"

int main(int argc, char *argv[]) {

  printf("Testing config file reading\n");

  char *config_path = "~/configurable_code/docs/example_config.json";

  printf("Config file path: %s\n", config_path);

  ControlData *control_data;
  Metadata *metadata;
  int ret = readConfigFile(config_path, &control_data, &metadata);
  if (ret != 0) {
    printf("Couldn't read the config\n");
    return -1;
  }

  if (strcmp(metadata->location, control_data->location)) {
    printf("mismatch locations: %s", control_data->location);
    free_Metadata(metadata);
    free_ControlData(control_data);
    return -1;
  }
  printf("Location: %s\n", metadata->location);
  printf("RTSP address: %s\n", control_data->RTSPaddress);
  printf("Base recording directory: %s\n", control_data->rawStorage);

  char *recording_dir;
  if (format_recording_directory(control_data, &recording_dir) != 0) {
    printf("Failed to format recording directory: %s\n", recording_dir);
    free(recording_dir);
    return -1;
  }
  printf("Recording directory: %s\n", recording_dir);
  free(recording_dir);

  printConfigSummary(control_data, metadata, control_data->head);

  free_Metadata(metadata);
  free_ControlData(control_data);

  char *rtsp_only = gst_pipeline_txt_gen(DEFAULT_PIPELINE_FILEPATH, 1);
  printf("RTSP only pipeline:\n%s\n", rtsp_only);
  free(rtsp_only);

  char *full_pipeline = gst_pipeline_txt_gen(DEFAULT_PIPELINE_FILEPATH, 0);
  printf("Full pipeline with raw recording:\n%s\n", full_pipeline);
  free(full_pipeline);

  return 0;
}