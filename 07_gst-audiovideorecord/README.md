# Audio Video Record

Record raw data from USB microphone and USB webcam or MIPI camera at the same time, then store them in MKV container.

![Figure audio video record pipeline](figure.png)

## Development Environment

GStreamer: 1.16.3 (edited by Renesas).

## Application Content

+ [`main.c`](main.c)

### Walkthrought
>Note that this tutorial only discusses the important points of this application. For the rest of source code, please refer to section [Audio Play](../01_gst-audioplay/README.md), [Audio Record](../05_gst-audiorecord/README.md) and [video Record](../06_gst-videorecord/README.md).

#### Output location
```
#define OUTPUT_FILE        "RECORD_Multimedia.mkv"
const gchar *output_file = OUTPUT_FILE;
```
#### Command-line argument
```
if (argc != ARG_COUNT) {
  g_print ("Error: Invalid arugments.\n");
  g_print ("Usage: %s <microphone device> <camera device> [width] [height]\n", argv[ARG_PROGRAM_NAME]);
  return -1;
}
```
This application accepts 2 command-line argument:
- The first points to USB microphone device card (hw:1,0, for example). Note: You can find this value by following section [Special Instruction](/05_gst-audiorecord/README.md).
- The second points to USB camera/MIPI camera device file (/dev/video9, for example). Note: You can find this value by following section [Special Instruction](/06_gst-videorecord/README.md).

Optional-argument width and height are used to set the resolution of camera.


#### Create elements
```
cam_src = gst_element_factory_make ("v4l2src", "cam-src");
cam_queue = gst_element_factory_make ("queue", "cam-queue");
cam_capsfilter = gst_element_factory_make ("capsfilter", "cam_caps");
video_conv_capsfilter =
    gst_element_factory_make ("capsfilter", "video-conv-caps");
video_encoder = gst_element_factory_make ("omxh264enc", "video-encoder");
video_parser = gst_element_factory_make ("h264parse", "h264-parser");

if (camera == MIPI_CAMERA) {
  video_converter = gst_element_factory_make ("vspmfilter", "video-converter");
} else {
  video_converter = gst_element_factory_make ("videoconvert", "video-converter");
}

audio_src = gst_element_factory_make ("alsasrc", "audio-src");
audio_queue = gst_element_factory_make ("queue", "audio-queue");
audio_converter = gst_element_factory_make ("audioconvert", "audio-conv");
audio_conv_capsfilter =
    gst_element_factory_make ("capsfilter", "audio-conv-caps");
audio_encoder = gst_element_factory_make ("vorbisenc", "audio-encoder");

muxer = gst_element_factory_make ("matroskamux", "mkv-muxer");
sink = gst_element_factory_make ("filesink", "file-output");

```
To record raw data from USB microphone and USB webcam or MIPI camera at the same time, then store them in MKV container, the following elements are used:
-	 Element v4l2src captures video from V4L2 devices.
-	 Element queue (cam_queue and audio_queue) queues data until one of the limits specified by the max-size-buffers, max-size-bytes, and/or max-size-time properties has been reached. Any attempt to push more buffers into the queue will block the pushing thread until more space becomes available.
-	 Element capsfilter specifies raw video format, framerate, and resolution.
-	 Element videoconvert (used for USB camera) and element vspmfilter (used for MIPI camera) converts video frames to a format (such as: NV12) understood by omxh264enc.
-	 Element omxh264enc encodes raw video into H.264 compressed data.
-	 Element h264parse connects omxh264enc to qtmux.
-	 Element alsasrc reads data from an audio card using the ALSA API.
-	 Element audioconvert converts raw audio buffers to a format (such as: F32LE) understood by vorbisenc.
-	 Element vorbisenc encodes raw audio into a Vorbis stream.
-	 Element matroskamux merges audio stream and video stream to MKV container.
-	 Element filesink writes incoming data to a local file.

#### Set element’s properties
```
g_object_set (G_OBJECT (video_converter), "dmabuf-use", true, NULL);
g_object_set (G_OBJECT (video_encoder), "target-bitrate", MIPI_BITRATE_OMXH264ENC,
    "control-rate", VARIABLE_RATE, "interval_intraframes", 14, "periodicty-idr", 2, NULL);
g_object_set (G_OBJECT (video_encoder), "target-bitrate", USB_BITRATE_OMXH264ENC,
    "control-rate", VARIABLE_RATE, NULL);
g_object_set (G_OBJECT (cam_src), "device", argv[ARG_CAMERA], NULL);

g_object_set (G_OBJECT (audio_src), "device", argv[ARG_MICROPHONE], NULL);
g_object_set (G_OBJECT (audio_encoder), "bitrate", BITRATE_ALSASRC, NULL);

g_object_set (G_OBJECT (sink), "location", output_file, NULL);
```
The g_object_set() function is used to set some element’s properties, such as:
-	 The device property of v4l2src element which points to camera’s device file. Users will pass the device file as a command line argument to this application. Please refer to section [Video Record Special Instruction](/06_gst-videorecord/README.md) to find the value.
-	 The location property of filesink element which points to MKV output file.
-	 The dmabuf-use property of vspmfilter element which is set to true. This disallows dmabuf to be output buffer. If it is not set, the output file will be broken.
-	 The target-bitrate property of omxh264enc element is used to specify encoding bit rate. The higher bitrate, the better quality.
-	 The control-rate property of omxh264enc element is used to specify birate control method which is variable bitrate method in this case.
-	 The interval_intraframes property of omxh264enc element is used to specify interval of coding intra frames.
-	 The periodicty-idr property of omxh264enc is used to specify periodicity of IDR frames.
-	 The device property of alsasrc element which points to a microphone device. Users will pass the device card as a command line argument to this application. Please refer to section [Audio record Special Instruction](/05_gst-audiorecord/README.md) to find the value.
-  The bitrate property of vorbisenc element is used to specify encoding bit rate. The higher bitrate, the better quality.
```
/* MIPI camera*/
cam_caps = gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING, "UYVY",
            "width", G_TYPE_INT, width, "height", G_TYPE_INT, height, NULL);
/* USB camera */
cam_caps = gst_caps_new_simple ("video/x-raw", "width", G_TYPE_INT, width,
          "height", G_TYPE_INT, height, NULL);

video_conv_caps = gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING, F_NV12, NULL);
audio_conv_caps = gst_caps_new_simple ("audio/x-raw", "format", G_TYPE_STRING, F_F32LE,
    "channels", G_TYPE_INT, CHANNEL, "rate", G_TYPE_INT, SAMPLE_RATE, NULL);

g_object_set (G_OBJECT (cam_capsfilter), "caps", cam_caps, NULL);
g_object_set (G_OBJECT (video_conv_capsfilter), "caps", video_conv_caps, NULL);
g_object_set (G_OBJECT (audio_conv_capsfilter), "caps", audio_conv_caps, NULL);

gst_caps_unref (cam_caps);
gst_caps_unref (video_conv_caps);
gst_caps_unref (audio_conv_caps);
```
Capabilities (short: caps) describe the type of data which is streamed between two pads.
The gst_caps_new_simple() function creates new caps which holds these values. These caps are then added to caps property of capsfilter elements (g_object_set).
Note that both caps should be freed with gst_caps_unref() if they are not used anymore.

#### Build pipeline
```
gst_bin_add_many (GST_BIN (pipeline), cam_src, cam_queue, cam_capsfilter, video_converter,
    video_conv_capsfilter, video_encoder, video_parser, audio_src, audio_queue,
    audio_converter, audio_conv_capsfilter, audio_encoder, muxer, sink, NULL);

gst_element_link_many (cam_src, cam_queue, cam_capsfilter,
        video_converter, video_conv_capsfilter, video_encoder, video_parser,NULL);

gst_element_link_many (audio_src, audio_queue, audio_converter,
        audio_conv_capsfilter, audio_encoder, NULL);

gst_element_link (muxer, sink);
```
The reason for the separation is that the sink pad of matroskamux (muxer) cannot be created automatically but is only created on demand. This application uses self-defined function link_to_multiplexer() to link the sink pad to source pad of vorbisenc (encoder) and h264parse (parser). That’s why its sink pad is called Request Pad.
>Note that the order counts, because links must follow the data flow (this is, from source elements to sink elements).

### Link request pads
When oggdemux (demuxer) finally has enough information to start producing data, it will create source pads, and trigger the pad-added signal. At this point our callback will be called:
```
srcpad = gst_element_get_static_pad (video_parser, "src");
link_to_multiplexer (srcpad, muxer);
gst_object_unref (srcpad);

srcpad = gst_element_get_static_pad (audio_encoder, "src");
link_to_multiplexer (srcpad, muxer);
gst_object_unref (srcpad);

```
This block gets the source pad (srcpad) of vorbisenc (encoder) and h264parse (parser), then calls link_to_multiplexer() to link them to the sink pad of matroskamux (muxer).
>Note that the srcpad should be freed with gst_object_unref() if it is not used anymore.

```
static void link_to_multiplexer (GstPad * tolink_pad, GstElement * mux)
{
  pad = gst_element_get_compatible_pad (mux, tolink_pad, NULL);
  gst_pad_link (tolink_pad, pad);
    gst_object_unref (GST_OBJECT (pad));
}
```
This function uses gst_element_get_compatible_pad() to request a sink pad (pad) which is compatible with the source pad (tolink_pad) of matroskamux (mux), then calls gst_pad_link() to link them together.
>Note that the pad should be freed with gst_object_unref() if it is not used anymore.

### Play pipeline
```
gst_element_set_state (pipeline, GST_STATE_PLAYING);
```
Every pipeline has an associated [state](https://gstreamer.freedesktop.org/documentation/plugin-development/basics/states.html). To start audio recording, the pipeline needs to be set to PLAYING state.
```
signal (SIGINT, signalHandler);
```
This application will stop recording if user presses Ctrl-C. To do so, it uses signal() to bind SIGINT (interrupt from keyboard) to signalHandler().
To know how this function is implemented, please refer to the following code block:
```
void signalHandler (int signal)
{
  if (signal == SIGINT) {
    gst_element_send_event (pipeline, gst_event_new_eos ());
  }
}
```
It calls gst_element_send_event() to send EOS (End-of-Stream) signal (gst_event_new_eos) to the pipeline. This makes gst_bus_timed_pop_filtered() return. Finally, the program cleans up GStreamer objects and exits.

## How to Build and Run GStreamer Application

This section shows how to cross-compile and deploy GStreamer _audio play_ application.

### How to Extract SDK
Please refer to _hello word_ [README.md](../#00_gst-helloworld/README.md) for more details.

### How to Build and Run GStreamer Application

***Step 1***.	Go to gst-audiovideorecord directory:
```
$   cd $WORK/07_gst-audiovideorecord
```

***Step 2***.	Cross-compile:
```
$   make
```
***Step 3***.	Copy all files inside this directory to /usr/share directory on the target board:
```
$   scp -r $WORK/07_gst-audiovideorecord/ <username>@<board IP>:/usr/share/
```
***Step 4***.	Run the application:
```
$   /usr/share/07_gst-audiovideorecord/gst-audiovideorecord $(/usr/share/05_gst-audiorecord/detect_microphone.sh) $(/usr/share/06_gst-videorecord/detect_camera.sh) <width> <height>
```
>For more details about _detect_microphone.sh_ and detect_camera.sh script. Please refer to [Audio record special instruction](/05_gst-audiorecord/README.md) and [Video record special instruction](/06_gst-videorecord/README.md)
### Special instruction:
#### Recommended USB cameras:
Option 1: Logitech USB HD Webcam C270 (model: V-U0018).

Option 2: Logitech USB HD 1080p Webcam C930E.

Option 3: Logitech USB UHD Webcam BRIO.

#### Recommended MIPI camera:
MIPI Mezzanine Adapter and OV5645 camera.
### To check the output file:
Option 1: VLC media player (https://www.videolan.org/vlc/index.html).

Option 2: Tool gst-launch-1.0 (on board):

```
$ gst-launch-1.0 filesrc location=RECORD_Multimedia.mkv ! matroskademux name=d d. ! queue max-size-time=10000000000 ! vorbisdec ! audioconvert ! audioresample ! autoaudiosink d. ! queue ! h264parse ! omxh264dec ! waylandsink
```