# RZ/V2H Group GStreamer Sample Code

This is a GStreamer Sample Code that provided for the RZ/V2H Group of 64bit Arm-based MPUs from Renesas Electronics.

## Evaluation Environment

Renesas evaluates all source codes here in the environment as below:

1. AI SDK
    * [RZ/V2H AI SDK v5.20](https://renesas-rz.github.io/rzv_ai_sdk/5.10/getting_started.html)

2. Boards and MPUs
    * Board: RZ/V2H Evaluation Board Kit / MPU: R9A09G057H4 (RZ/V2H)

## LICENSE

Each [Application Samples](#application-samples) (including [Hello World](#hello-world)) covered by respective LICENSE.

Each LICENSE is placed within each [Application Samples](#application-samples) (including [Hello World](#hello-world)) directory.

# GStreamer Applications

[GStreamer](https://gstreamer.freedesktop.org/) is a library for constructing graphs of media-handling components. The applications it supports range from simple Ogg/Vorbis playback, audio/video streaming to complex audio (mixing) and video (non-linear editing) processing.

GStreamer is released under the LGPL. This section explains how to create and run the GStreamer applications on Wayland Window System. The applications can take advantage of codec, filter technology, and hardware processing of RZ/V2H SoC by using Renesas GStreamer elements: `omxh264dec`, `omxh265dec`, `omxh264enc`, `omxh265enc`, `vspmfilter`, `waylandsink`.

Before you proceed to [Application Samples](#application-samples) section, please make sure to check [Hello World](00_gst-helloworld) README.

## Application Samples

The following table shows multimedia applications in RZ/V2H platforms ranging from playing, recording, scaling, streaming audio/video, to displaying multiple videos on multiple monitors.

| Application Name | Description |
| ---------------- | ----------- |
| [Audio Play](01_gst-audioplay) | Play an MP3 audio file. |
| [Video Play](02_gst-videoplay) | Play an H.264 or H.265 video file. |
| [Audio Encode](03_gst-audioencode) | Encode audio data from F32LE raw format to Ogg/Vorbis format. |
| [Video Encode](04_gst-videoencode) | Encode video data from NV12 raw format to H.264 format. |
| [Audio Record](05_gst-audiorecord) | Record raw data from USB microphone, then store it in Ogg container. |
| [Video Record](06_gst-videorecord) | Display and record raw video from USB/MIPI camera, then store it in MP4 container. |
| [Audio Video Record](07_gst-audiovideorecord) | Record raw data from microphone and MIPI camera or USB webcam at the same time, then store them in MKV container. |
| [Receive Streaming Video](08_gst-receivestreamingvideo) | Receive and display streaming video. |
| [Send Streaming Video](09_gst-sendstreamingvideo) | Send streaming video. |
| [Video Scale](10_gst-videoscale) | Scale down an H.264 video, then store it in MP4 container. |
| [Audio Player](11_gst-audioplayer) | A simple text-based MP3 audio player. |
| [Video Player](12_gst-videoplayer) | A simple text-based MP4 video player. |
| [Audio Video Play](13_gst-audiovideoplay) | Play H.264 video and MP3 audio file independently. |
| [File Play](14_gst-fileplay) | Play an MP4 file. |
| [Multiple Displays 1](15_gst-multipledisplays1) | Display 1 H.264 or H.265 video simultaneously on HDMI monitor. |
| [Multiple Displays 2](16_gst-multipledisplays2) | Display 2 H.264 or H.265 videos simultaneously on HDMI monitor. |
| [Overlapped Display](17_gst-lappeddisplay) | Display 3 overlapping H.264 videos. |
