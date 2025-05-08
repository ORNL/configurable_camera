// Test this file with "nc -ulzp 6250"
#include <gio/gio.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/vfs.h>
#include <time.h>

#include "log.h"
#include "config.h"
#include "statusbroadcaster.h"
#include "cJSON.h"

#define TEMP_FACTOR 1000
#define MAX_TEMP_LEN 6
#define GIGABYTES 1000000000

char *CPU_TEMP_FILE = "/sys/devices/virtual/thermal/thermal_zone0/temp";


static gint readCPUTemp(void) {
  FILE *fp;
  char *buffer;
  buffer = (char*)malloc(sizeof(char)*6);

  fp = fopen (CPU_TEMP_FILE, "r" );
  if (!fp) {
    return -1;
  }

  int temp = atoi(fgets(buffer, MAX_TEMP_LEN, fp)) / TEMP_FACTOR;

  fclose(fp);
  free(buffer);
  return temp;
}

gboolean setup_socket(StatusBroadcast *status_broadcast) {
  // Function to set up the socket that broadcasts status info from the Jetson
  PipelineData *pipeline_data = status_broadcast->pipeline_data;
  ControlData *control_data = pipeline_data->control_data;

  char *broadcast_addr = control_data->broadcastAddress;

  GError *socket_error = NULL;
  GInetAddress *iaddr = g_inet_address_new_from_string(broadcast_addr);
  GSocketAddress *address = g_inet_socket_address_new(
    iaddr, control_data->statusBroadcastPort);
  GSocket *socket = g_socket_new(G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_DATAGRAM,
                                 G_SOCKET_PROTOCOL_UDP, &socket_error);
  g_socket_set_broadcast(socket, TRUE);

  if (g_socket_get_broadcast(socket)) {
    timestamp_prefix_log();
    g_print("Broadcasting on: %s:%i\n", broadcast_addr,
      control_data->statusBroadcastPort);
  } else {
    timestamp_prefix_err();
    g_printerr("Failed to broadcast status on: %s:%i\n", broadcast_addr, 
      control_data->statusBroadcastPort);
  }

  if (socket_error != NULL) {
    timestamp_prefix_err();
    g_printerr("%s", socket_error->message);
    return FALSE;
  }

  status_broadcast->socket = socket;
  status_broadcast->address = address;
  return TRUE;
}

static char *formatStatusBroadcastJSON(StatusBroadcast *sb) {

  PipelineData *pipeline_data = sb->pipeline_data;
  SensorDynamicInfo *sensor_info = sb->sensor_info;
  ControlData *control_data = pipeline_data->control_data;
  char *json_string;
  cJSON *root, *hardware_list, *hardware;

  root = cJSON_CreateObject();
  cJSON_AddItemToObject(root, "Name", cJSON_CreateString(sb->metadata->name));
  cJSON_AddItemToObject(root, "Hostname", cJSON_CreateString(control_data->hostname));
  cJSON_AddItemToObject(root, "IP Address", cJSON_CreateString(control_data->controlIP));
  cJSON_AddItemToObject(root, "Control Port", cJSON_CreateNumber(control_data->controlPort));
  cJSON_AddItemToObject(root, "Location", cJSON_CreateString(control_data->location));
  cJSON_AddItemToObject(root, "Range (m)", cJSON_CreateNumber(sb->metadata->range));
  cJSON_AddItemToObject(root, "Lens", cJSON_CreateString(sb->metadata->lensName));
  cJSON_AddItemToObject(root, "Recording Directory", cJSON_CreateString(control_data->rawStorage));
  cJSON_AddItemToObject(root, "RTSP Address", cJSON_CreateString(control_data->RTSPaddress));

  if (sensor_info != NULL) {
    cJSON_AddItemToObject(root, "Sensor", cJSON_CreateString(sensor_info->name));
    cJSON_AddItemToObject(root, "Sensor Temperature", cJSON_CreateNumber(sensor_info->temperature));
    cJSON_AddItemToObject(root, "Exposure Time", cJSON_CreateNumber(sensor_info->exposure_time));
    cJSON_AddItemToObject(root, "Gain", cJSON_CreateNumber(sensor_info->gain));
    if (sensor_info->temperature_state == TEMPERATURE_OK) {
      cJSON_AddItemToObject(root, "Sensor Temperature State", cJSON_CreateString("OK"));
    } else if (sensor_info->temperature_state == TEMPERATURE_CRITICAL) {
      cJSON_AddItemToObject(root, "Sensor Temperature State", cJSON_CreateString("CRITICAL"));
    } else if (sensor_info->temperature_state == TEMPERATURE_ERROR) {
      cJSON_AddItemToObject(root, "Sensor Temperature State", cJSON_CreateString("ERROR"));
    } else {
      cJSON_AddItemToObject(root, "Sensor Temperature State", cJSON_CreateString("BAD VALUE"));
    }
  } else {
    cJSON_AddItemToObject(root, "Sensor", cJSON_CreateString("Pipeline Inactive"));
    cJSON_AddItemToObject(root, "Sensor Temperature", cJSON_CreateNumber(0));
    cJSON_AddItemToObject(root, "Sensor Temperature", cJSON_CreateNumber(0));
    cJSON_AddItemToObject(root, "Exposure Time", cJSON_CreateNumber(0));
    cJSON_AddItemToObject(root, "Sensor Temperature State", cJSON_CreateString("OK"));
  }

  cJSON_AddItemToObject(root, "RTSP", cJSON_CreateBool(sb->pipeline_data->is_streaming));
  cJSON_AddItemToObject(root, "Raw", cJSON_CreateBool(sb->pipeline_data->is_raw_recording));
  cJSON_AddItemToObject(root, "Memory Remaining (GB)", cJSON_CreateNumber(sb->memory_remaining_gb));
  cJSON_AddItemToObject(root, "Disk Size (GB)", cJSON_CreateNumber(sb->disk_size_gb));
  cJSON_AddItemToObject(root, "CPU Temperature", cJSON_CreateNumber(sb->cpu_temp));
  cJSON_AddItemToObject(root, "Timestamp", cJSON_CreateNumber(sb->timestamp));

  hardware_list = cJSON_CreateArray();
  cJSON_AddItemToObject(root, "Hardware", hardware_list);

  for (HardwareScripts *hws = control_data->head; hws != NULL; hws = hws->next) {
    cJSON_AddItemToArray(hardware_list, hardware = cJSON_CreateObject());
    cJSON_AddItemToObject(hardware, "Name", cJSON_CreateString(hws->name));
    cJSON_AddItemToObject(hardware, "Control Port", cJSON_CreateNumber(atoi(hws->listenerPort)));
    // shallow copy of JSON from the config file
    cJSON_AddItemReferenceToObject(hardware, "Capabilities", hws->capabilities);
  }

  json_string = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);
  free(sb->sensor_info);
  sb->sensor_info = NULL;
  return json_string;
}


gboolean status_broadcaster(gpointer data) {
  // Callback to periodically send Jetson status back to control laptop

  static int counter = 0;
  StatusBroadcast *status_broadcast = data;
  PipelineData *pipeline_data = status_broadcast->pipeline_data;
  ControlData *control_data = pipeline_data->control_data;
  freeSensorDynamicInfo(status_broadcast->sensor_info);
  if(pipeline_data->is_streaming) {
    status_broadcast->sensor_info = getSensorDynamicInfo(pipeline_data->source);
  } else {
    freeSensorDynamicInfo(status_broadcast->sensor_info);
  }

  // Get stats on filesystem
  struct statfs stats;

  statfs(control_data->rawStorage, &stats);

  // Figure out how many GB are left on drive, put it in a string
  long total_space = (stats.f_bsize) * (stats.f_blocks) / GIGABYTES;
  long free_space = (stats.f_bsize) * (stats.f_bfree) / GIGABYTES;
  
  status_broadcast->memory_remaining_gb = free_space;
  status_broadcast->disk_size_gb = total_space;
  status_broadcast->cpu_temp = readCPUTemp();
  status_broadcast->timestamp = time(NULL);

  char *json_string = formatStatusBroadcastJSON(status_broadcast);

  gsize chars_written = strlen(json_string);

  // Send the UDP packet
  GError *send_error = NULL;
  g_socket_send_to(status_broadcast->socket, status_broadcast->address, json_string,
                   chars_written, NULL, &send_error);

  free(json_string);

  if (send_error != NULL) {
    timestamp_prefix_err();
    g_printerr("%s", send_error->message);
  }
  counter++;

  // TODO: remove this timeout from the loop once quit command comes through
  return TRUE;
}
