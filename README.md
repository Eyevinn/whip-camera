<h1 align="center">
  Eyevinn WHIP Camera
</h1>
<div align="center">
Web camera ingestion client for <a href="https://www.ietf.org/archive/id/draft-ietf-wish-whip-01.html">WHIP</a>
  <br/>
</div>

<div align="center">
  <br/>

  [![github release](https://img.shields.io/github/v/release/Eyevinn/whip-camera?style=flat-square)](https://github.com/Eyevinn/whip-camera/releases)
  [![license](https://img.shields.io/github/license/eyevinn/whip-camera.svg?style=flat-square)](LICENSE)
  [![PRs welcome](https://img.shields.io/badge/PRs-welcome-ff69b4.svg?style=flat-square)](https://github.com/eyevinn/whip-camera/issues?q=is%3Aissue+is%3Aopen+label%3A%22help+wanted%22)
  [![made with hearth by Eyevinn](https://img.shields.io/badge/made%20with%20%E2%99%A5%20by-Eyevinn-59cbe8.svg?style=flat-square)](https://github.com/eyevinn)
  [![Slack](http://slack.streamingtech.se/badge.svg)](http://slack.streamingtech.se)

</div>

Ingests a web camera stream and sends it to a WHIP endpoint.

The program will choose an appropriate video source to use. It currently does not support choosing between several appropriate sources.

## Install Binary

Homebrew:

```
brew install eyevinn/tools/whip-camera
```

## Build from source

### Mac OS

Requirements:
- XCode command line tools installed
- Install additional dependencies using homebrew

Install dependencies:
```
brew install gstreamer gst-plugins-bad gst-plugins-good libsoup@2 icu4c cmake gst-libav
```

On Apple M1 you might need to build the gst-plugins-bad from source as the SRT plugins are not available in the binary bottle.
```
brew reinstall --build-from-source gst-plugins-bad
```

```
cmake -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" .
make
```

### Ubuntu 22.04

Requirements:
- Install dependencies

Install dependencies:
```
apt-get install libgstreamer1.0-0 gstreamer1.0-plugins-bad gstreamer1.0-plugins-good gstreamer1.0-libav gstreamer1.0-plugins-rtp gstreamer1.0-nice libsoup2.4-1 cmake gcc g++ make gdb libglib2.0-dev libgstreamer1.0-dev libgstreamer-plugins-bad1.0-dev libsoup2.4-dev
```

```
cmake -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" .
make
```

## Usage

```
Usage: GST_PLUGIN_PATH=[GST_PLUGIN_PATH] ./whip-camera [OPTION]
  -b, buffer INT
  -u, whipUrl STRING
  -s, sourceDevice STRING(linux) or INT(Mac OS)
  -l

Options:
  -b set duration to buffer in the jitterbuffers (in ms)
  -u url address for WHIP endpoint
  -s set the video source device. Use -l to list sources to see which devices are detected. Leaving this option unset uses autovideosrc to automatically identify a suitable device.
  -l list video source devices with video/x-raw capabilities
```

Example Mac OS - run program
```
GST_PLUGIN_PATH=/usr/local/lib/gstreamer-1.0 ./whip-camera -b 50 -u "http://myWhipURL" -s 0 
```

Example Linux - run program
```
GST_PLUGIN_PATH=/usr/local/lib/gstreamer-1.0 ./whip-camera -b 50 -u "http://myWhipURL" -s "/dev/video0" 
```

Example - list sources
```
GST_PLUGIN_PATH=/usr/local/lib/gstreamer-1.0 ./whip-camera -l 
```

To run you need to set the `GST_PLUGIN_PATH` environment variable to where you have the gstreamer plugins installed, e.g:

```
export GST_PLUGIN_PATH=/usr/local/lib/gstreamer-1.0
```

Some OS(like newer macs) have the following path:

```
export GST_PLUGIN_PATH=/opt/homebrew/lib/gstreamer-1.0
```

## Support

Join our [community on Slack](http://slack.streamingtech.se) where you can post any questions regarding any of our open source projects. Eyevinn's consulting business can also offer you:

- Further development of this component
- Customization and integration of this component into your platform
- Support and maintenance agreement

Contact [sales@eyevinn.se](mailto:sales@eyevinn.se) if you are interested.

## About Eyevinn Technology

[Eyevinn Technology](https://www.eyevinntechnology.se) is an independent consultant firm specialized in video and streaming. Independent in a way that we are not commercially tied to any platform or technology vendor. As our way to innovate and push the industry forward we develop proof-of-concepts and tools. The things we learn and the code we write we share with the industry in [blogs](https://dev.to/video) and by open sourcing the code we have written.

Want to know more about Eyevinn and how it is to work here. Contact us at work@eyevinn.se!
