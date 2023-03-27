<h1 align="center">
  Eyevinn Whip-Camera
</h1>
<div align="center">
Web camera ingestion client for WHIP (https://github.com/Eyevinn/whip).
  <br/>
  <br/>
</div>

<div align="center">
  <br/>

  [![npm](https://img.shields.io/npm/v/eyevinn-channel-engine?style=flat-square)](https://www.npmjs.com/package/eyevinn-channel-engine)
  [![github release](https://img.shields.io/github/v/release/Eyevinn/channel-engine?style=flat-square)](https://github.com/Eyevinn/channel-engine/releases)
  [![license](https://img.shields.io/github/license/eyevinn/channel-engine.svg?style=flat-square)](LICENSE)
  [![PRs welcome](https://img.shields.io/badge/PRs-welcome-ff69b4.svg?style=flat-square)](https://github.com/eyevinn/channel-engine/issues?q=is%3Aissue+is%3Aopen+label%3A%22help+wanted%22)
  [![made with hearth by Eyevinn](https://img.shields.io/badge/made%20with%20%E2%99%A5%20by-Eyevinn-59cbe8.svg?style=flat-square)](https://github.com/eyevinn)
  [![Slack](http://slack.streamingtech.se/badge.svg)](http://slack.streamingtech.se)

</div>

Ingests a web camera stream and sends it to a WHIP endpoint.

The program will choose an appropriate video source to use. It currently does not support choosing between several appropriate sources.

## Build from source

### Mac OS

Requirements:
- XCode command line tools installed
- Install additional dependencies using homebrew

```
brew install gstreamer gst-plugins-bad gst-plugins-good libsoup@2 icu4c cmake gst-libav
```

```
cmake -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" .
make
```

### Ubuntu 22.04

Requirements:
- Install dependencies

```
apt-get install libgstreamer1.0-0 gstreamer1.0-plugins-bad gstreamer1.0-plugins-good gstreamer1.0-libav gstreamer1.0-plugins-rtp gstreamer1.0-nice libsoup2.4-1 cmake gcc g++ make gdb libglib2.0-dev libgstreamer1.0-dev libgstreamer-plugins-bad1.0-dev libsoup2.4-dev
```

```
cmake -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" .
make
```

## Run

To run you need to set the `GST_PLUGIN_PATH` environment variable to where you have the gstreamer plugins installed, e.g:

```
export GST_PLUGIN_PATH=/usr/local/lib/gstreamer-1.0
```

Some OS(like newer macs) have the following path:

```
export GST_PLUGIN_PATH=/opt/homebrew/lib/gstreamer-1.0
```

Then run the command.
```
./whip-camera
```

Flags:
- -b set duration to buffer in the jitterbuffers (in ms)
- -u url address for WHIP endpoint
- -l list video source devices with video/x-raw capabilities

run example
```
GST_PLUGIN_PATH=/usr/local/lib/gstreamer-1.0 ./whip-camera -b 50 -u "http://myWhipURL" 
```

list video sources example
```
GST_PLUGIN_PATH=/usr/local/lib/gstreamer-1.0 ./whip-camera -l 
```

## License (Apache-2.0)

```
Copyright 2022 Eyevinn Technology AB

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
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
