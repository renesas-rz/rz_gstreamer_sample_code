# RZ/G2L Group and RZ/V2L Group GStreamer Sample Code

This is a GStreamer Sample Code that provided for the RZ/G2L Group and RZ/V2L Group of 64bit Arm-based MPUs from Renesas Electronics.

## Supported VLP (Verified Linux Package)

Currently the following VLP versions are supported:

* [RZ/G Verified Linux Package [5.10-CIP]](https://www.renesas.com/us/en/products/microcontrollers-microprocessors/rz-mpus/rzg-linux-platform/rzg-marketplace/verified-linux-package/rzg-verified-linux-package)
* [RZ/V Verified Linux Package [5.10-CIP]](https://www.renesas.com/us/en/software-tool/rzv-verified-linux-package)

## Supported Boards and MPUs

Currently the following boards and MPUs are supported:

* Board: RZG2L SMARC Evaluation Kit / MPU: R9A07G044L (RZ/G2L)
* Board: RZV2L SMARC Evaluation Kit / MPU: R9A07G054L (RZ/V2L)

## LICENSE

Each [Application Samples](#application-samples) (including [Hello World](#hello-world)) covered by respective LICENSE.

Each LICENSE is placed within each [Application Samples](#application-samples) (including [Hello World](#hello-world)) directory.

## Disclaimer

All sample code here is only provided and tested for specific boards and MPUs with specific VLP versions (here, called platform).

Renesas does not guarantee all sample code provided will run on different platforms.

# GStreamer Applications

[GStreamer](https://gstreamer.freedesktop.org/) is a library for constructing graphs of media-handling components. The applications it supports range from simple Ogg/Vorbis playback, audio/video streaming to complex audio (mixing) and video (non-linear editing) processing.

GStreamer is released under the LGPL. This section explains how to create and run the GStreamer applications on Wayland Window System. The applications can take advantage of codec, filter technology, and hardware processing of RZ/G2L & RZ/V2L SoC by using Renesas GStreamer elements: `omxh264dec`, `omxh264enc`, `vspmfilter`, `waylandsink`.

## Hello World

Before you proceed to [Application Samples](#application-samples) Section, please make sure to check [Hello World](00_gst-helloworld) README.

## Application Samples

The following table shows multimedia applications in RZ/G2 platform ranging from playing, recording, scaling, streaming audio/video, to displaying multiple videos on multiple monitors.

| Application Name | Description |
| ---------------- | ----------- |
| [Audio Play](01_gst-audioplay) | Play an Ogg/Vorbis audio file. |
| [Video Play](02_gst-videoplay) | Play an H.264 video file. |
| [Audio Encode](03_gst-audioencode) | Encode audio data from F32LE raw format to Ogg/Vorbis format. |
| [Video Encode](04_gst-videoencode) | Encode video data from NV12 raw format to H.264 format. |
| [Audio Record](05_gst-audiorecord) | Record raw data from USB microphone, then store it in Ogg container. |
| [Video Record](06_gst-videorecord) | Display and record raw video from USB/MIPI camera, then store it in MP4 container. |
| [Audio Video Record](07_gst-audiovideorecord) | Record raw data from USB microphone and USB/MIPI camera at the same time, then store them in MKV container. |
| [Receive Streaming Video](08_gst-receivestreamingvideo) | Receive and display streaming video. |
| [Send Streaming Video](09_gst-sendstreamingvideo) | Send streaming video. |
| [Video Scale](10_gst-videoscale) | Scale down an H.264 video, then store it in MP4 container. |
| [Audio Player](11_gst-audioplayer) | A simple text-based Ogg/Vorbis audio player. |
| [Video Player](12_gst-videoplayer) | A simple text-based MP4 video player. |
| [Audio Video Play](13_gst-audiovideoplay) | Play H.264 video and Ogg/Vorbis audio file independently. |
| [File Play](14_gst-fileplay) | Play an MP4 file. |
| [Multiple Displays 1](15_gst-multipledisplays1) | Display 1 H.264 video simultaneously on HDMI monitors. |
| [Multiple Displays 2](16_gst-multipledisplays2) | Display 2 H.264 videos simultaneously on HDMI monitors. |
| [Overlapped Display](17_gst-lappeddisplay) | Display 3 overlapping H.264 videos. |
