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

int32_t main(int32_t argc, char** argv)
{
    gst_init(nullptr, nullptr);
    g_main_loop_unref(mainLoop);
    gst_deinit();

    printf("Hello world\n");

    return 0;
}