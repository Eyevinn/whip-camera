#define GST_USE_UNSTABLE_API 1 // Removes compile warning

#include "http/WhipClient.h"
#include "nlohmann/json.hpp"
#include <csignal>
#include <getopt.h>
#include <glib.h>
#include <gst/gst.h>
#include <gst/sdp/sdp.h>
#include <gst/webrtc/webrtc.h>
#include <map>
#include <string.h>

namespace
{
GMainLoop* mainLoop = nullptr;

std::map<std::string, GstElement*> elements_;

const char* usageString = "Usage: GST_PLUGIN_PATH=[GST_PLUGIN_PATH] ./whip-camera [OPTION]\n"
                          "  -b, buffer INT\n"
                          "  -u, whipUrl STRING\n"
                          "  -s, sourceDevice STRING(linux) or INT(Mac OS)\n"
                          "  -l\n"
                          "\n"
                          "Options:\n"
                          "-b set duration to buffer in the jitterbuffers (in ms)\n"
                          "-u url address for WHIP endpoint\n"
                          "-s set the video source device. Use -l to list sources to see which devices are detected. "
                          "Leaving this option unset uses autovideosrc to automatically identify a suitable device.\n"
                          "-l list video source devices with video/x-raw capabilities\n";

std::string whipResource_;
std::string etag_;
std::string sourceDevice_;
std::string sourceElement_;

GstElement* pipeline_ = nullptr;

GstCaps* rtpVideoFilterCaps_ = nullptr;
GstCaps* rtpAudioFilterCaps_ = nullptr;
} // namespace

void makeElement(GstElement* pipeline, const char* elementLabel, const char* element);
void buildAndLinkPipelineElements(http::WhipClient& whipClient, std::string buffer);
GstDeviceMonitor* setupRawVideoSourceDeviceMonitor();
void onNegotiationNeededCallback(GstElement* src, http::WhipClient& whipClient);
void onOfferCreatedCallback(GstPromise* promise, gpointer whipClient);

void intSignalHandler(int32_t)
{
    g_main_loop_quit(mainLoop);
}

int32_t main(int32_t argc, char** argv)
{
    gst_init(nullptr, nullptr);

    uint32_t buffer = 0;
    const char* whipUrl = nullptr;
    int32_t getOptResult;

    while ((getOptResult = getopt(argc, argv, "b:lu:s:")) != -1)
        switch (getOptResult)
        {
        case 'b':
            buffer = std::strtoul(optarg, nullptr, 10);
            break;
        case 'l':
            setupRawVideoSourceDeviceMonitor();
            return 0;
        case 'u':
            whipUrl = optarg;
            break;
        case 's':
            sourceDevice_ = optarg;
            break;
        default:
            break;
        }

    if (whipUrl == nullptr)
    {
        printf("%s\n", usageString);
        printf("Example: GST_PLUGIN_PATH=/usr/local/lib/gstreamer-1.0 ./whip-camera -b 50 -u "
               "'http://localhost:8200/api/v2/whip/sfu-broadcaster?channelId=test'\n\n");
        return 1;
    }

    http::WhipClient whipClient(whipUrl, "");
    buildAndLinkPipelineElements(whipClient, std::to_string(buffer));

    {
        struct sigaction sigactionData = {};
        sigactionData.sa_handler = intSignalHandler;
        sigactionData.sa_flags = 0;
        sigemptyset(&sigactionData.sa_mask);
        sigaction(SIGINT, &sigactionData, nullptr);
    }

    printf("Start playing...\n");
    if (gst_element_set_state(pipeline_, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
    {
        printf("Unable to set the pipeline to the playing state\n");
        return 1;
    }

    printf("Looping...\n");
    mainLoop = g_main_loop_new(nullptr, FALSE);
    g_main_loop_run(mainLoop);

    g_main_loop_unref(mainLoop);
    gst_element_set_state(pipeline_, GST_STATE_NULL);
    if (pipeline_)
    {
        printf("\nUnref pipeline...\n");
        g_object_unref(pipeline_);
    }
    gst_deinit();
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

    if (!gst_bin_add(GST_BIN(pipeline), result))
    {
        printf("Failed to add element %s\n", elementLabel);
        return;
    }
}

GstDeviceMonitor* setupRawVideoSourceDeviceMonitor()
{
    GstDeviceMonitor* monitor;
    GstCaps* caps;

    monitor = gst_device_monitor_new();

    caps = gst_caps_new_empty_simple("video/x-raw");
    gst_device_monitor_add_filter(monitor, "Video/Source", caps);
    gst_caps_unref(caps);

    auto devices = gst_device_monitor_get_devices(monitor);
    if (devices != nullptr)
    {
        auto i = 0;
        while (devices != nullptr)
        {
            auto device = reinterpret_cast<GstDevice*>(devices->data);
            printf("%d: ", i);
            printf("%s\n", gst_device_get_display_name(device));
            auto deviceProps = gst_device_get_properties(device);
            auto path = gst_structure_get_string(deviceProps, "object.path");
            if (path)
            {
                printf("%s\n", path);
            }

            gst_object_unref(device);
            gst_structure_free(deviceProps);
            devices = g_list_delete_link(devices, devices);
            i++;
        }
    }

    gst_device_monitor_start(monitor);

    return monitor;
}

void onNegotiationNeededCallback(GstElement* src, http::WhipClient& whipClient)
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

    auto promise = gst_promise_new_with_change_func(onOfferCreatedCallback, &whipClient, nullptr);
    g_signal_emit_by_name(elements_["webrtcbin"], "create-offer", nullptr, promise);
}

void onOfferCreatedCallback(GstPromise* promise, gpointer whipClientAddress)
{
    printf("onOfferCallback\n");

    GstWebRTCSessionDescription* offerDesc;
    auto whipClient = reinterpret_cast<http::WhipClient*>(whipClientAddress);
    const auto reply = gst_promise_get_reply(promise);
    gst_structure_get(reply, "offer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &offerDesc, nullptr);
    gst_promise_unref(promise);

    const auto offerString = std::string(gst_sdp_message_as_text(offerDesc->sdp));

    auto sendOfferReply = whipClient->sendOffer(offerString);
    if (sendOfferReply.resource_.empty())
    {
        printf("whipClient sendOffer failed\n");
        return;
    }

    whipResource_ = std::move(sendOfferReply.resource_);
    etag_ = std::move((sendOfferReply.etag_));

    printf("Server responded with resource %s, etag %s\n", whipResource_.c_str(), etag_.c_str());
    printf("Setting local SDP\n");
    g_signal_emit_by_name(elements_["webrtcbin"], "set-local-description", offerDesc, nullptr);

    GstSDPMessage* answerMessage = nullptr;

    if (gst_sdp_message_new_from_text(sendOfferReply.sdpAnswer_.c_str(), &answerMessage) != GST_SDP_OK)
    {
        printf("Unable to create SDP object from answer\n");
        return;
    }
    printf("Created SDP object from answer\n");

    GstWebRTCSessionDescription* answer(gst_webrtc_session_description_new(GST_WEBRTC_SDP_TYPE_ANSWER, answerMessage));
    if (!answer)
    {
        printf("Unable to create SDP object from answer\n");
        return;
    }

    printf("Setting remote SDP\n");
    g_signal_emit_by_name(elements_["webrtcbin"], "set-remote-description", answer, nullptr);
}

void buildAndLinkPipelineElements(http::WhipClient& whipClient, std::string buffer)
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
    rtpAudioFilterCaps_ = gst_caps_new_simple("application/x-rtp",
        "media",
        G_TYPE_STRING,
        "audio",
        "payload",
        G_TYPE_INT,
        111,
        "encoding-name",
        G_TYPE_STRING,
        "OPUS",
        nullptr);

    if (!sourceDevice_.empty())
    {
#if __APPLE__
        sourceElement_ = "avfvideosrc";
        makeElement(pipeline_, "camerasource", sourceElement_.c_str());
        g_object_set(elements_["camerasource"], "device-index", std::stoi(sourceDevice_), nullptr);
#elif __linux__
        sourceElement_ = "v4l2src";
        makeElement(pipeline_, "camerasource", sourceElement_.c_str());
        g_object_set(elements_["camerasource"], "device", sourceDevice_.c_str(), nullptr);
#endif
    }
    else
    {
        makeElement(pipeline_, "camerasource", "autovideosrc");
    }

    makeElement(pipeline_, "videoconvert", "videoconvert");
    makeElement(pipeline_, "vp8enc", "vp8enc");
    makeElement(pipeline_, "rtpvp8pay", "rtpvp8pay");
    makeElement(pipeline_, "rtp_video_payload_queue", "queue");

    makeElement(pipeline_, "audiotestsrc", "audiotestsrc");

    makeElement(pipeline_, "audioconvert", "audioconvert");
    makeElement(pipeline_, "audioresample", "audioresample");
    makeElement(pipeline_, "opusenc", "opusenc");
    makeElement(pipeline_, "rtpopuspay", "rtpopuspay");
    makeElement(pipeline_, "rtp_audio_payload_queue", "queue");

    makeElement(pipeline_, "webrtcbin", "webrtcbin");
    g_object_set(elements_["webrtcbin"],
        "name",
        "send",
        "stun-server",
        "stun://stun.l.google.com:19302",
        "bundle-policy",
        GST_WEBRTC_BUNDLE_POLICY_MAX_BUNDLE,
        "latency",
        buffer.c_str(),
        nullptr);
    gst_element_sync_state_with_parent(elements_["webrtcbin"]);
    g_signal_connect(elements_["webrtcbin"],
        "on-negotiation-needed",
        G_CALLBACK(onNegotiationNeededCallback),
        &whipClient);

    if (!gst_element_link_filtered(elements_["rtp_video_payload_queue"], elements_["webrtcbin"], rtpVideoFilterCaps_))
    {
        printf("rtp_video_payload_queue-webrtcbin could not be linked\n");
        return;
    }

    if (!gst_element_link_many(elements_["camerasource"],
            elements_["videoconvert"],
            elements_["vp8enc"],
            elements_["rtpvp8pay"],
            nullptr))
    {
        printf("camerasource-videoconvert-vp8enc-rtpvp8pay could not be linked\n");
        return;
    }

    if (!gst_element_link_many(elements_["rtpvp8pay"], elements_["rtp_video_payload_queue"], nullptr))
    {
        printf("rtpvp8pay-rtp_video_payload_queue could not be linked\n");
        return;
    }

    if (!gst_element_link_filtered(elements_["rtp_audio_payload_queue"], elements_["webrtcbin"], rtpAudioFilterCaps_))
    {
        printf("rtp_audio_payload_queue-webrtcbin could not be linked\n");
        return;
    }

    if (!gst_element_link_many(elements_["audiotestsrc"],
            elements_["audioconvert"],
            elements_["audioresample"],
            elements_["opusenc"],
            elements_["rtpopuspay"],
            elements_["rtp_audio_payload_queue"],
            nullptr))
    {
        printf("audiotestsrc-audioconvert-audioresample-opusenc-rtpopuspay-rtp_audio_payload_queue could not be "
               "linked\n");
        return;
    }
}