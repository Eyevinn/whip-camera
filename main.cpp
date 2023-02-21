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
void intSignalHandler(int32_t)
{
    g_main_loop_quit(mainLoop);
}

int32_t main(int32_t argc, char** argv)
{
    printf("init\n");
    setenv("GST_PLUGIN_PATH", "/usr/local/lib/gstreamer-1.0", 0);
    gst_init(nullptr, nullptr);
    void padAdded(GstElement * src, GstPad * newPad, Connection * connection);

    Connection connection;

    // Create elements
    connection.pipeline_ = gst_pipeline_new("pipeline");

    connection.sourceElement_ = gst_element_factory_make("autovideosrc", "source");
    if (!connection.sourceElement_)
    {
        printf("Failed to make element source. Note: GST_PLUGIN_PATH needs to be set as described in the README.\n");
        return 1;
    }

    connection.sinkElement_ = gst_element_factory_make("webrtcbin", "sink");
    if (!connection.sinkElement_)
    {
        printf("Failed to make element sink\n");
        return 1;
    }

    // Add elements
    if (!gst_bin_add(GST_BIN(connection.pipeline_), connection.sourceElement_))
    {
        printf("Failed to add element source. Note: GST_PLUGIN_PATH needs to be set as described in the README\n");
        return 1;
    }

    if (!gst_bin_add(GST_BIN(connection.pipeline_), connection.sinkElement_))
    {
        printf("Failed to add element gli_sink\n");
        return 1;
    }

    // Signals
    g_signal_connect(connection.sourceElement_, "pad-added", G_CALLBACK(padAdded), &connection);

    {
        struct sigaction sigactionData = {};
        sigactionData.sa_handler = intSignalHandler;
        sigactionData.sa_flags = 0;
        sigemptyset(&sigactionData.sa_mask);
        sigaction(SIGINT, &sigactionData, nullptr);
    }

    // Start playing
    printf("Start playing...\n");
    if (gst_element_set_state(connection.pipeline_, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
    {
        printf("Unable to set the pipeline to the playing state\n");
        return 1;
    }

    printf("Looping...\n");
    mainLoop = g_main_loop_new(nullptr, FALSE);
    g_main_loop_run(mainLoop);

    // Release gst
    g_main_loop_unref(mainLoop);
    gst_element_set_state(connection.pipeline_, GST_STATE_NULL);
    gst_deinit();
    printf("deinit\n");
    return 0;
}

void padAdded(GstElement* src, GstPad* newPad, Connection* connection)
{
    printf("Received new pad '%s' from '%s'\n", GST_PAD_NAME(newPad), GST_ELEMENT_NAME(src));

    if (!gst_element_link_many(connection->sourceElement_, connection->sinkElement_, nullptr))
    {
        printf("Failed to link source to sink\n");
    }
}