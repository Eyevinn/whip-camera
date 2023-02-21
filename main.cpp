#define GST_USE_UNSTABLE_API 1// Removes compile warning

#include "nlohmann/json.hpp"
#include <csignal>
#include <cstdint>
#include <glib.h>
#include <gst/gst.h>
#include <gst/sdp/sdp.h>
#include <gst/webrtc/webrtc.h>
#include <iostream>
#include <libsoup/soup.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Connection
{
    GstElement* pipeline_;
    GstElement* sourceElement_;
    GstElement* sinkElement_;

    Connection()
        : pipeline_(nullptr),
          sourceElement_(nullptr),
          sinkElement_(nullptr)
    {
    }

    ~Connection()
    {
        printf("\nDestructing resources...\n");
        if (pipeline_)
        {
            g_object_unref(pipeline_);
        }
    }
};

GMainLoop* mainLoop = nullptr;

int32_t main(int32_t argc, char** argv)
{
    printf("init\n");
    setenv("GST_PLUGIN_PATH", "/usr/local/lib/gstreamer-1.0", 0);
    setenv("GST_V4L2_USE_LIBV4L2", "1", 0);
    gst_init(nullptr, nullptr);
    mainLoop = g_main_loop_new(nullptr, FALSE);

    Connection connection;

    //Create elements
    connection.pipeline_ = gst_pipeline_new("pipeline");

    connection.sourceElement_ = gst_element_factory_make("autovideosrc", "source");
    if (!connection.sourceElement_)
    {
        printf("Failed to make element source. Note: GST_PLUGIN_PATH needs to be set as described in the README.\n");
        return 1;
    }

    connection.sinkElement_ = gst_element_factory_make("autovideosink", "sink");
    if (!connection.sinkElement_)
    {
        printf("Failed to make element sink\n");
        return 1;
    }

    //Release gst
    g_main_loop_unref(mainLoop);
    gst_deinit();
    printf("deinit\n");
    return 0;
}