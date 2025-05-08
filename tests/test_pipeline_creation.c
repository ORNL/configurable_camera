
#include <gst/gst.h>
#include "sensor.h"
#include "pipeline.h"


int main(int argc, char *argv[]){
    GstElement *sensor, *pipeline;
    GError *error;
    SensorSetupInfo *sensor_setup_info;

    GstRTSPServer *server;
    GstRTSPMountPoints *mounts;
    GstRTSPMediaFactory *factory;

    PipelineData pipeline_data;

    gst_init(&argc, &argv);

    GError *parse_error = NULL;

    pipeline_data.loop = g_main_loop_new(NULL, FALSE);
    server = gst_rtsp_server_new();
    gst_rtsp_server_set_address(server, "192.168.0.101");
    mounts = gst_rtsp_server_get_mount_points(server);
    factory = gst_rtsp_media_factory_new();

    gchar *base_pipeline_string = 
        "( pylonsrc name=source ! capsfilter name=capsfilter ! "
        "tee name=t "
        "t. ! queue name=raw_queue ! fakesink name=sink "
        "t. ! queue name=rtsp_queue ! nvvidconv name=converter ! video/x-raw(memory:NVMM),format=I420 ! "
        "nvv4l2h264enc ! h264parse ! rtph264pay name=pay0 )";

    gst_rtsp_media_factory_set_launch(factory, base_pipeline_string);
    gst_rtsp_media_factory_set_shared(factory, TRUE);    

    g_signal_connect(factory, "media-configure", (GCallback) media_configure, (gpointer) &pipeline_data);

    gst_rtsp_mount_points_add_factory(mounts, "/test", factory);
    g_object_unref(mounts);

    gst_rtsp_server_attach(server, NULL);

    g_print("stream ready at rtsp://127.0.0.1:8554/test\n");

    g_main_loop_run(pipeline_data.loop);

}