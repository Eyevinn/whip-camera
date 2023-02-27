#define GST_USE_UNSTABLE_API 1// Removes compile warning

#include "http/WhipClient.h"
#include "nlohmann/json.hpp"
#include <csignal>
#include <cstdint>
#include <glib.h>
#include <gst/gst.h>
#include <gst/sdp/sdp.h>
#include <gst/webrtc/webrtc.h>
#include <iostream>
#include <libsoup/soup.h>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace http
{
class WhipClient;
}

GMainLoop* mainLoop = nullptr;
struct Connection;
std::map<std::string, GstElement*> elements_;

void padAddedCallback(GstElement* src, GstPad* newPad, Connection* connection);
void onNegotiationNeededCallback(GstElement* src, Connection* connection);
void onOfferCreatedCallback(GstPromise* promise, gpointer userData);
void makeElement(GstElement* pipeline, const char* elementLabel, const char* element);

struct Connection
{
    GstElement* pipeline_;
    GstCaps* rtpVideoFilterCaps_;
    http::WhipClient& whipClient_;

    Connection(http::WhipClient& whipClient)
        : pipeline_(nullptr),
          rtpVideoFilterCaps_(nullptr),
          whipClient_(whipClient)
    {
        pipeline_ = gst_pipeline_new("pipeline");
        rtpVideoFilterCaps_ = gst_caps_new_simple("application/x-rtp",
                "media",
                G_TYPE_STRING,
                "video",
                "payload",
                G_TYPE_INT,
                96,
                "encoding-name",
                G_TYPE_STRING,
                "VP8",
                nullptr);

        makeElement(pipeline_, "camerasource", "autovideosrc");
        g_signal_connect(elements_["camerasource"], "pad-added", G_CALLBACK(padAddedCallback), this);

        makeElement(pipeline_, "vp8enc", "vp8enc");
        makeElement(pipeline_, "videoconvert", "videoconvert");
        makeElement(pipeline_, "rtpvp8pay", "rtpvp8pay");
        makeElement(pipeline_, "rtp_video_payload_queue", "queue");

        makeElement(pipeline_, "webrtcbin", "webrtcbin");
        g_object_set(elements_["webrtcbin"], "name", "send", "stun-server", "stun://stun.l.google.com:19302", nullptr);
        gst_element_sync_state_with_parent(elements_["webrtcbin"]);
        g_signal_connect(elements_["webrtcbin"],
                "on-negotiation-needed",
                G_CALLBACK(onNegotiationNeededCallback),
                this);
        g_signal_connect(elements_["webrtcbin"], "pad-added", G_CALLBACK(onNegotiationNeededCallback), this);

        if (!gst_element_link_filtered(elements_["rtp_video_payload_queue"],
                    elements_["webrtcbin"],
                    rtpVideoFilterCaps_))
        {
            printf("queue could not be linked to webrtcbin\n");
            return;
        }
        printf("Queue successfully linked to webrtcbin\n");

        if (!gst_element_link_many(elements_["camerasource"],
                    elements_["videoconvert"],
                    elements_["vp8enc"],
                    elements_["rtpvp8pay"],
                    nullptr))
        {
            printf("Source elements could not be linked\n");
            return;
        }
        printf("Successfully linked source elements\n");

        if (!gst_element_link_many(elements_["rtpvp8pay"], elements_["rtp_video_payload_queue"], nullptr))
        {
            printf("Final link not possible\n");
            return;
        }
        printf("Final link established\n");
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
    //setenv("GST_DEBUG", "4", 0);
    gst_init(nullptr, nullptr);

    http::WhipClient whipClient("whipEndpointUrl", "whipEndpointAuthKey");
    Connection connection(whipClient);

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

    // Release Gstreamer resources
    g_main_loop_unref(mainLoop);
    gst_element_set_state(connection.pipeline_, GST_STATE_NULL);
    gst_deinit();
    printf("deinit\n");
    return 0;
}

void makeElement(GstElement* pipeline, const char* elementLabel, const char* element)
{
    const auto& result = gst_element_factory_make(element, elementLabel);
    elements_.emplace(elementLabel, result);
    if (!result)
    {
        printf("Unable to make gst element %s", element);
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

void padAddedCallback(GstElement* src, GstPad* newPad, Connection* connection)
{
    printf("Received new pad '%s' from '%s'\n", GST_PAD_NAME(newPad), GST_ELEMENT_NAME(src));
}

void onNegotiationNeededCallback(GstElement* src, Connection* connection)
{
    printf("onNegotiationNeededCallback\n");

    GArray* transceivers;
    g_signal_emit_by_name(elements_["webrtcbin"], "get-transceivers", &transceivers);

    for (uint32_t i = 0; i < transceivers->len; ++i)
    {
        auto transceiver = g_array_index(transceivers, GstWebRTCRTPTransceiver*, i);
        if (g_object_class_find_property(G_OBJECT_GET_CLASS(transceiver), "direction") != nullptr)
        {
            g_object_set(transceiver, "direction", GST_WEBRTC_RTP_TRANSCEIVER_DIRECTION_SENDONLY, nullptr);
        }
        g_object_set(transceiver, "fec-type", GST_WEBRTC_FEC_TYPE_NONE, nullptr);
        g_object_set(transceiver, "do-nack", TRUE, nullptr);
    }

    auto promise = gst_promise_new_with_change_func(onOfferCreatedCallback, connection, nullptr);
    g_signal_emit_by_name(elements_["webrtcbin"], "create-offer", nullptr, promise);
}

void onOfferCreatedCallback(GstPromise* promise, gpointer userData)
{
    printf("onOfferCallback\n");

    GstWebRTCSessionDescription* offerDesc;
    const auto reply = gst_promise_get_reply(promise);
    gst_structure_get(reply, "offer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &offerDesc, nullptr);
    gst_promise_unref(promise);

    //const auto offerString = std::string(gst_sdp_message_as_text(offerDesc->sdp));

    // auto sendOfferReply = whipClient_.sendOffer(offerString);
    // if (sendOfferReply.resource_.empty())
    // {
    //     return;
    // }
}