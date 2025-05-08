#ifndef STATUSBROADCASTER_H
#define STATUSBROADCASTER_H

#include <gio/gio.h>
#include <gst/gst.h>

#include <time.h>

#include "config.h"
#include "sensor.h"
#include "pipeline.h"

#define STATUS_TIMEOUT 5

typedef struct StatusBroadcast {
  GSocket *socket;
  GSocketAddress *address;

  gint memory_remaining_gb;
  gint disk_size_gb;
  gint cpu_temp;
  time_t timestamp;

  SensorDynamicInfo *sensor_info;
  Metadata *metadata;
  PipelineData *pipeline_data;
} StatusBroadcast;

gboolean setup_socket(StatusBroadcast *status_broadcast);

gboolean status_broadcaster(gpointer data);

#endif