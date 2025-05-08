#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <gst/gst.h>

typedef struct UDPListener {
  GSocket *socket;
  GSocketAddress *address;
  GIOChannel *channel;
} UDPListener;

typedef struct ParsedTrigger {
  gchar *entire_string;
  gchar *command;
  gchar *label;
  gchar *timestamp;
} ParsedTrigger;


UDPListener *startUDPListener(ControlData *control_data);

void stopUDPListener(UDPListener *udp_listener);

gboolean udp_received_callback(GIOChannel *source, GIOCondition condition, gpointer data);

#endif
