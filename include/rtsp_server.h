#ifndef RTSP_SERVER_H
#define RTSP_SERVER_H

#include <gst/rtsp-server/rtsp-server.h>

void media_configure(GstRTSPMediaFactory *factory, GstRTSPMedia *media, gpointer user_data);

void cleanup_RTSP_media(PipelineData *pipeline_data);

void client_connected(GstRTSPServer *server, GstRTSPClient *client, gpointer user_data);

#endif
