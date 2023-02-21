#define GST_USE_UNSTABLE_API 1// Removes compile warning

//#include "Connection.h"
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

int32_t main(int32_t argc, char** argv)
{
    printf("Hello world\n");
    setenv("GST_PLUGIN_PATH", "/usr/local/lib/gstreamer-1.0", 0);
    gst_init(nullptr, nullptr);
    mainLoop = g_main_loop_new(nullptr, FALSE);
    g_main_loop_unref(mainLoop);
    gst_deinit();
    printf("deinit\n");
    return 0;
}