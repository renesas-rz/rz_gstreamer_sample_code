# RZ GStreamer Sample Code

This is a GStreamer Sample Code that provided for the RZ Family MPUs from Renesas Electronics.

## Evaluation Environment

All code provided in this repository is provided "as is" and is designed to be easy to use and compatible with newer software (BSP/VLP/SDK) provided by Renesas (via Renesas.com).<br>
Renesas makes no warranties, express or implied, and assumes no liability whatsoever if the code does not function correctly or does not meet any of the descriptions.<br>
Renesas evaluates all source codes here in the environment as below:

| Index | Board | MPU | BSP/VLP/SDK |
|-------|-------|-----|-------------|
| 1 | RZ/G2L SMARC Evaluation Kit | R9A07G044L (RZ/G2L) | [RZ/G Verified Linux Package [5.10-CIP]](https://www.renesas.com/us/en/products/microcontrollers-microprocessors/rz-mpus/rzg-linux-platform/rzg-marketplace/verified-linux-package/rzg-verified-linux-package) Version 3.0.7-update3 <sup>[1]</sup> <br>[RZ MPU Verified Linux Package [6.1-CIP]](https://www.renesas.com/en/software-tool/rz-mpu-verified-linux-package-61-cip) Version 4.0.0 <sup>[2]</sup> |
| 2 | RZ/V2L SMARC Evaluation Kit | R9A07G054L (RZ/V2L) | [RZ/V Verified Linux Package [5.10-CIP]](https://www.renesas.com/us/en/software-tool/rzv-verified-linux-package) Version 3.0.7-update3 <sup>[1]</sup> <br>[RZ/V2L AI SDK v7.00](https://renesas-rz.github.io/rzv_ai_sdk/7.00/getting_started.html) <sup>[2]</sup> |
| 3 | RZ/V2N Evaluation Board Kit | R9A09G056N44 (RZ/V2N) | [RZ/V2N AI SDK v5.00](https://renesas-rz.github.io/rzv_ai_sdk/5.10/getting_started.html) <sup>[1]</sup><br>[RZ/V2N AI SDK v6.00](https://renesas-rz.github.io/rzv_ai_sdk/6.00/getting_started.html) <sup>[2]</sup> |
| 4 | RZ/V2H Evaluation Board Kit | R9A09G057H4 (RZ/V2H) | [RZ/V2H AI SDK v5.20](https://renesas-rz.github.io/rzv_ai_sdk/5.10/getting_started.html) <sup>[1]</sup><br>[RZ/V2H AI SDK v6.00](https://renesas-rz.github.io/rzv_ai_sdk/6.20/getting_started.html) <sup>[2]</sup> |
| 5 | RZ/G3E SMARC Evaluation Kit | R9A09G047E57 (RZ/G3E) | [RZ/G3E Board Support Package](https://www.renesas.com/en/software-tool/rzg3e-board-support-package) Version 1.0.0 <sup>[2]</sup> |

<sup>[1]</sup> Linux kernel version 5.10 and Yocto version 3.1 (Dunfell)\
<sup>[2]</sup> Linux kernel version 6.1 and Yocto version 5.0 (Scrathgap)
## LICENSE

Each [Application Samples](#application-samples) (including [Hello World](00_gst-helloworld)) covered by respective LICENSE.

Each LICENSE is placed within each [Application Samples](#application-samples) (including [Hello World](00_gst-helloworld)) directory.

# GStreamer Applications

[GStreamer](https://gstreamer.freedesktop.org/) is a library for constructing graphs of media-handling components. The applications it supports range from simple Ogg/Vorbis playback, audio/video streaming to complex audio (mixing) and video (non-linear editing) processing.

GStreamer is released under the LGPL. This section explains how to create and run the GStreamer applications on Wayland Window System. The applications can take advantage of codec, filter technology, and hardware processing of RZ Family MPUs by using Renesas GStreamer elements: `omxh264dec`, `omxh265dec`, `omxh264enc`, `omxh265enc`, `vspmfilter`, `waylandsink`.

**Note:**

* `omxh265dec` and `omxh265enc` are only for supported MPUs;
* The MPUs that support these elements are RZ/V2N, RZ/V2H, RZ/G3E MPUs.

Before you proceed to [Application Samples](#application-samples) section, please make sure to check [Hello World](00_gst-helloworld) README.

## Application Samples

The following table shows multimedia applications in supported MPUs ranging from playing, recording, scaling, streaming audio/video, to displaying multiple videos on multiple monitors.

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
| [Video Scale](10_gst-videoscale) | Scale an H.264 video, then store it in MP4 container. |
| [Audio Player](11_gst-audioplayer) | A simple text-based MP3 audio player. |
| [Video Player](12_gst-videoplayer) | A simple text-based MP4 video player. |
| [Audio Video Play](13_gst-audiovideoplay) | Play H.264 video and MP3 audio file independently. |
| [File Play](14_gst-fileplay) | Play an MP4 file. |
| [Multiple Displays 1](15_gst-multipledisplays1) | Display 1 H.264 or H.265 video simultaneously on HDMI monitor. |
| [Multiple Displays 2](16_gst-multipledisplays2) | Display 2 H.264 or H.265 videos simultaneously on HDMI monitor. |
| [Overlapped Display](17_gst-lappeddisplay) | Display 3 overlapping H.264 videos. |

**Note:**
> H.265 in the Application Samples is only for MPUs that support `omxh265dec` and `omxh265enc`.

# Release

Sample Code can be access either from branch or tag.

Latest Sample Code (August 2025 and later) is prepared in this main branch.

Tag name is combination of BSP/VLP/AI-SDK version and specific MPU (of EVK).

See all releases [here](https://github.com/renesas-rz/rz_gstreamer_sample_code/tags).

## Existing Sample Code Prior to August 2025 Release (Older Release)

Existing Sample Code branches and/or tags can be accessed as before.

### Specific Boards and MPUs Sample Code

To get Sample Code for your boards and MPUs, please access specific link below:

- Linux kernel version 6.1 and Yocto version 5.0 (Scrathgap):
    1. [RZ/G2L Group](../vlp-4.0.x_rz-g2l)
        * Board: RZ/G2L SMARC Evaluation Kit / MPU: R9A07G044L (RZ/G2L)

- Linux kernel version 5.10 and Yocto version 3.1 (Dunfell):
    1. [RZ/G2L Group and RZ/V2L Group](../vlp-3.0.x_rz-g2l_rz-v2l)
        * Board: RZ/G2L SMARC Evaluation Kit / MPU: R9A07G044L (RZ/G2L)
        * Board: RZ/V2L SMARC Evaluation Kit / MPU: R9A07G054L (RZ/V2L)
    2. [RZ/V2N Group](../ai-sdk-5.xx_rz-v2n)
        * Board: RZ/V2N Evaluation Board Kit / MPU: R9A09G056N44 (RZ/V2N)
    3. [RZ/V2H Group](../ai-sdk-5.xx_rz-v2h)
        * Board: RZ/V2H Evaluation Board Kit / MPU: R9A09G057H4 (RZ/V2H)

### Layout

This GStreamer Sample Code provided as:

```
<this project>
 |- <combination of VLP/AI-SDK version with boards and MPUs group target branch>
     |- <sample code for each use-case folder>
```

Documentation for each use-case exist on each folder.

### Release

Sample Code can be access either from branch or tag.

Branches are prepared for combination of VLP/AI-SDK version with boards and MPUs group.

Tag name is combination of VLP/AI-SDK version and specific MPU (of EVK).

See all releases [here](https://github.com/renesas-rz/rz_gstreamer_sample_code/tags).
