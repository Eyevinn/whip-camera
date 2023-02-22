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

GMainLoop* mainLoop = nullptr;
struct Connection;
void padAdded(GstElement* src, GstPad* newPad, Connection* connection);
void onNegotiationNeededCallback(GstElement* src, Connection* connection);
void makeElement(GstElement* pipeline, const char* elementLabel, const char* element);

struct Connection
{
    GstElement* pipeline_;
    GstElement* cameraSourceElement_;
    GstElement* webrtcElement_;
    GstElement* videoConvertElement_;
    GstElement* vp8encElement_;
    GstElement* rtpvp8payElement_;

    Connection()
        : pipeline_(nullptr),
          cameraSourceElement_(nullptr),
          webrtcElement_(nullptr),
          videoConvertElement_(nullptr),
          vp8encElement_(nullptr),
          rtpvp8payElement_(nullptr)
    {
        pipeline_ = gst_pipeline_new("pipeline");

        makeElement(pipeline_, "cameraSource", "autovideosrc");
        makeElement(pipeline_, "webrtcbin", "webrtcbin");
        makeElement(pipeline_, "videoConvert", "videoconvert");
        makeElement(pipeline_, "vp8enc", "vp8enc");
        makeElement(pipeline_, "rtpvp8pay", "rtpvp8pay");
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

void intSignalHandler(int32_t)
{
    g_main_loop_quit(mainLoop);
}

int32_t main(int32_t argc, char** argv)
{
    printf("init\n");
    setenv("GST_PLUGIN_PATH", "/usr/local/lib/gstreamer-1.0", 0);
    gst_init(nullptr, nullptr);

    Connection connection;

    // Signals
    g_signal_connect(connection.webrtcElement_,
            "on-negotiation-needed",
            G_CALLBACK(onNegotiationNeededCallback),
            &connection);

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

void makeElement(GstElement* pipeline, const char* elementLabel, const char* element)
{
    const auto result = gst_element_factory_make(element, elementLabel);
    if (!result)
    {
        printf("Failed to make element %s\n", elementLabel);
        return;
    }
    printf("Made element %s\n", elementLabel);

    if (!gst_bin_add(GST_BIN(pipeline), result))
    {
        printf("Failed to add element %s\n", elementLabel);
        return;
    }
    printf("Added element %s\n", elementLabel);
}

void padAdded(GstElement* src, GstPad* newPad, Connection* connection)
{
    printf("Received new pad '%s' from '%s'\n", GST_PAD_NAME(newPad), GST_ELEMENT_NAME(src));

    if (!gst_element_link_many(connection->cameraSourceElement_, connection->webrtcElement_, nullptr))
    {
        printf("Failed to link source to sink\n");
    }
}

void onNegotiationNeededCallback(GstElement* src, Connection* connection)
{
    printf("onNegotiationNeededCallback\n");
    //do things like create offer
}