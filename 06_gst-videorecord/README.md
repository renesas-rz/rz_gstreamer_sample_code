# Video Record

Display and encode raw data from USB/MIPI camera to H.264 format, then store it in MP4 container when user presses Ctrl-C.

![Figure video record pipeline](figure.png)

## Development Environment

GStreamer: 1.16.3 (edited by Renesas).

## Application Content

+ [`main.c`](main.c)

### Walkthrought
>Note that this tutorial only discusses the important points of this application. For the rest of source code, please refer to section [Audio Play](../01_gst-audioplay/README.md).
#### Output location
```c
const gchar *output_file = "RECORD-camera.mp4";
```
#### Command-line argument
```c
if (argc != ARG_COUNT) {
  g_print ("Error: Invalid arugments.\n");
  g_print ("Usage: %s <camera device> [width] [height]\n", argv[ARG_PROGRAM_NAME]);
  return -1;
}
camera = check_camera_type (argv[ARG_DEVICE]);
```
This application accepts a command-line argument which points to camera’s device file (/dev/video9, for example). The type of camera is determined by function check_camera_type. Note: You can find this value by following section [Special Instruction](#special-instruction)

User can enter width and height options to set camera’s resolution.

#### Create elements
```c
source = gst_element_factory_make ("v4l2src", "camera-source");
camera_capsfilter = gst_element_factory_make ("capsfilter", "camera_caps");
convert_capsfilter = gst_element_factory_make ("capsfilter", "convert_caps");
queue1 = gst_element_factory_make ("queue", "queue1");
encoder = gst_element_factory_make ("omxh264enc", "video-encoder");
parser = gst_element_factory_make ("h264parse", "h264-parser");
muxer = gst_element_factory_make ("qtmux", "mp4-muxer");
filesink = gst_element_factory_make ("filesink", "file-output");

if (camera == MIPI_CAMERA) {
  converter = gst_element_factory_make ("vspmfilter", "video-converter");
} else {
  converter = gst_element_factory_make ("videoconvert", "video-converter");
}

tee = gst_element_factory_make ("tee", "tee");
queue2 = gst_element_factory_make ("queue", "queue2");
waylandsink = gst_element_factory_make ("waylandsink", "video-output");
```
To display and record camera then store it in MP4 container, the following elements are used:
-	 Element v4l2src captures video from V4L2 devices.
-	 Element capsfilter specifies raw video format, framerate, and resolution.
-	 Element videoconvert (used for USB camera) and element vspmfilter (used for MIPI camera) convert video frames to a format (such as: NV12) understood by omxh264enc.
-	 Element tee splits (video) data to multiple pads.
-	 Element queue (queue1 and queue2) queues data until one of the limits specified by the max-size-buffers, max-size-bytes, and/or max-size-time properties has been reached. Any attempt to push more buffers into the queue will block the pushing thread until more space becomes available.
-	 Element omxh264enc encodes raw video into H.264 compressed data.
-	 Element h264parse connects omxh264enc to qtmux.
-	 Element qtmux merges H.264 byte stream to MP4 container.
-	 Element filesink writes incoming data to a local file.
-	 Element waylandsink creates its own window and renders the decoded video frames to that.

#### Set element’s properties
```c
g_object_set (G_OBJECT (source), "device", argv[ARG_DEVICE], NULL);
g_object_set (G_OBJECT (filesink), "location", output_file, NULL);

/*MIPI camera*/
g_object_set (G_OBJECT (converter), "dmabuf-use", true, NULL);
/* Set properties of the encoder element - omxh264enc */
g_object_set (G_OBJECT (encoder), "target-bitrate", MIPI_BITRATE_OMXH264ENC,
    "control-rate", VARIABLE_RATE, "interval_intraframes", 14,
    "periodicty-idr", 2, "use-dmabuf", true, NULL);

/*USB camera*/
g_object_set (G_OBJECT (encoder), "target-bitrate", USB_BITRATE_OMXH264ENC,
    "control-rate", VARIABLE_RATE, NULL);
```
The _g_object_set()_ function is used to set some element’s properties, such as:
-	 The device property of v4l2src element which points to a camera device file. Users will pass the device file as a command line argument to this application. Please refer to section [Special Instruction](#special-instruction) to find the value.
-	 The location property of filesink element which points to MP4 output file.
-	 The dmabuf-use property of vspmfilter element which is set to true. This disallows dmabuf to be output buffer. If it is not set, waylandsink will display broken video frames.
-	 The target-bitrate property of omxh264enc element is used to specify encoding bit rate. The higher bitrate, the better quality.
-	 The control-rate property of omxh264enc element is used to specify birate control method which is variable bitrate method in this case.
-	 The interval_intraframes property of omxh264enc element is used to specify interval of coding intra frames.
-	 The periodicty-idr property of omxh264enc is used to specify periodicity of IDR frames.
```c
/*MIPI camera*/
camera_caps = gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING, "UYVY",
        "width", G_TYPE_INT, width, "height", G_TYPE_INT, height, NULL);

/*USB camera*/
camera_caps = gst_caps_new_simple ("video/x-raw", "width", G_TYPE_INT, width,
        "height", G_TYPE_INT, height, NULL);

convert_caps = gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING, "NV12", NULL);

g_object_set (G_OBJECT (camera_capsfilter), "caps", camera_caps, NULL);
g_object_set (G_OBJECT (convert_capsfilter), "caps", convert_caps, NULL);

gst_caps_unref (camera_caps);
gst_caps_unref (convert_caps);
```
Capabilities (short: caps) describe the type of data which is streamed between two pads. This data includes raw video format, resolution, and framerate.

In this application, two caps are required, one specifies video resolution captured from v4l2src, the other specifies video format (NV12) generated from converter.

The _gst_caps_new_simple()_ function creates new caps which holds these values. These caps are then added to caps property of capsfilter elements (g_object_set).

>Note that both camera_caps and convert_caps should be freed with _gst_caps_unref()_ if they are not used anymore.

#### Build pipeline
```c
gst_bin_add_many (GST_BIN (pipeline), source, camera_capsfilter, converter,
    convert_capsfilter, queue1, encoder, parser, muxer, filesink, NULL);

/*Not display video on monitor*/
gst_element_link_many (source, camera_capsfilter, converter,
          convert_capsfilter, queue1, encoder, parser, NULL);

/*Display video on monitor*/
gst_bin_add_many (GST_BIN (pipeline), tee, queue2, waylandsink, NULL);
gst_element_link_many (source, camera_capsfilter, converter,
          convert_capsfilter, tee, NULL);
gst_element_link_many (tee, queue1, encoder, parser, NULL);
gst_element_link_many (tee, queue2, waylandsink, NULL);

gst_element_link (muxer, filesink);
```
In case of not displaying video on monitor, this code block adds elements to pipeline and then links them into separated groups as below:
-	 Group #1: source, camera_capsfilter, converter, convert_capsfilter, queue1, encoder and parser.
-	 Group #2: muxer and filesink.
The reason for the separation is that the sink pad of qtmux (muxer) cannot be created automatically but is only created on demand. This application uses self-defined function _link_to_multiplexer()_ to link the sink pad to source pad of h264parse (parser). That’s why its sink pad is called Request Pad.
Note that the order counts, because links must follow the data flow (this is, from source elements to sink elements).

In case of displaying video on monitor, this code block adds elements to pipeline and then links them into separated groups as below:
-	 Group #1: source, camera_capsfilter, converter, convert_capsfilter and tee.
-	 Group #2: queue1, encoder and parser.
-	 Group #3: muxer and filesink.
-	 Group #4: queue2 and waylandsink.
Note that the order counts, because links must follow the data flow (this is, from source elements to sink elements).

### Link request pads
```c
srcpad = gst_element_get_static_pad (parser, "src");
link_to_multiplexer (srcpad, muxer);
gst_object_unref (srcpad);
```
This block gets the source pad (srcpad) of h264parse (parser), then calls _link_to_multiplexer()_ to link it to the sink pad of _qtmux_ (muxer).

>Note that the _srcpad_ should be freed with _gst_object_unref()_ if it is not used anymore.
```c
static void link_to_multiplexer (GstPad * tolink_pad, GstElement * mux)
{
  pad = gst_element_get_compatible_pad (mux, tolink_pad, NULL);
  gst_pad_link (tolink_pad, pad);

  gst_object_unref (GST_OBJECT (pad));
}
```
This function uses _gst_element_get_compatible_pad()_ to request a sink pad (pad) which is compatible with the source pad (tolink_pad) of qtmux (mux), then calls _gst_pad_link()_ to link them together.

>Note that the _pad_ should be freed with _gst_object_unref()_ if it is not used anymore.

### Play pipeline
```c
gst_element_set_state (pipeline, GST_STATE_PLAYING);
```
Every pipeline has an associated [state](https://gstreamer.freedesktop.org/documentation/plugin-development/basics/states.html). To start webcam recording, the pipeline needs to be set to PLAYING state.

```c
signal (SIGINT, signalHandler);
```
This application will stop recording if user presses Ctrl-C. To do so, it uses _signal()_ to bind SIGINT (interrupt from keyboard) to _signalHandler()_.

To know how this function is implemented, please refer to the following code block:

```c
void signalHandler (int signal)
{
  if (signal == SIGINT) {
    gst_element_send_event (pipeline, gst_event_new_eos ());
  }
}
```
It calls _gst_element_send_event()_ to send EOS (End-of-Stream) signal (gst_event_new_eos) to the pipeline. This makes _gst_bus_timed_pop_filtered()_ return. Finally, the program cleans up GStreamer objects and exits.

## How to Build and Run GStreamer Application

This section shows how to cross-compile and deploy GStreamer _video record_ application.

### How to Extract SDK
Please refer to _hello word_ [README.md](/00_gst-helloworld/README.md) for more details.

### How to Build and Run GStreamer Application

***Step 1***.	Go to gst-videorecord directory:
```sh
$   cd $WORK/06_gst-videorecord
```
***Step 2***.	Cross-compile:
```sh
$   make
```
***Step 3***.	Copy all files inside this directory to /usr/share directory on the target board:
```sh
$   scp -r $WORK/06_gst-videorecord/ <username>@<board IP>:/usr/share/
```
***Step 4***.  Setup MIPI camera (With USB camera you can skip this step):
```sh
$   /usr/share/06_gst-videorecord/setup_MIPI_camera.sh <width>x<height>
```
For more detail about _setup_MIPI_camera.sh_ script at [Special instruction](#special-instruction)
>Note: RZ/G2L and RZ/V2L only support 2 resolution for MIPI camera: 1920x1080, 1280x960

***Step 5***.	Run the application:
```sh
$   /usr/share/06_gst-videorecord/gst-videorecord $(/usr/share/06_gst-videorecord/detect_camera.sh) <width> <height>
```
>Note: Please enter the output width and height same as the resolution when initializing MIPI camera.
### Special instruction:
#### Reference USB Camera:
Logitech USB HD Webcam C270 (model: V-U0018), Logitech USB HD 1080p, Webcam C930E and Logitech USB UHD Webcam BRIO.
#### Reference MIPI Camera:
MIPI Mezzanine Adapter and OV5645 camera.
#### Run the following script to find camera device file:
```sh
$   ./detect_camera.sh
```
Basically, this script uses v4l2-ctl tool to read all information of device files (/dev/video8, for example) and find out if the device file has “Crop Capability Video Capture”. If the string is exist, the device file is available to use.
>This script can be used in combination with gst-videorecord application.
```sh
$   ./gst-videorecord $( ./detect_camera.sh ) <width> <height>
```
For further information on how this script is implemented, please refer to the following code block:
```sh
#!/bin/bash

ERR_NO_CAMERA=1
PROG_SUCCESS_CODE=0

PROG_STAT=$PROG_SUCCESS_CODE

for DEV_NAME in $( ls -v /dev/video* )

do
  CHECK_CAMERA=$( v4l2-ctl -d $DEV_NAME --all | grep "Video Capture:" )

  if [ ! -z "$CHECK_CAMERA" ]
  then
    CAMERA_DEV=$DEV_NAME
    echo $CAMERA_DEV
    break
  fi
done

if [ -z $CAMERA_DEV ]
then
  PROG_STAT=$ERR_NO_CAMERA
fi

exit $PROG_STAT
```
#### Run the following script to initialize MIPI camera:

```sh
./setup_MIPI_camera.sh <width>x<height>
```
Basically, this script uses media-ctl tool to set up format of camera device. For RZ/G2L platform, 2 acceptable resolutions are 1280x960, 1920x1080.

For further information on how this script is implemented, please refer to the following code block:

```sh
#!/bin/bash

if [[ $1 != "1280x960" ]] && [[ $1 != "1920x1080" ]]; then
	echo "RZG2L and RZV2L only support 2 camera resolutions"
	echo -e "1. 1920x1080\n2. 1280x960"
else
        media-ctl -d /dev/media0 -r
        media-ctl -d /dev/media0 -V "'ov5645 0-003c':0 [fmt:UYVY8_2X8/$1 field:none]"
        media-ctl -d /dev/media0 -l "'rzg2l_csi2 10830400.csi2':1 -> 'CRU output':0 [1]"
        media-ctl -d /dev/media0 -V "'rzg2l_csi2 10830400.csi2':1 [fmt:UYVY8_2X8/$1 field:none]"
	echo "/dev/video0 is configured successfully with resolution "$1""
fi
```
#### To check the output file:

Option 1: VLC media player (https://www.videolan.org/vlc/index.html).

Option 2: Tool gst-launch-1.0 (on board):
```sh
$ gst-launch-1.0 filesrc location=/home/media/videos/RECORD_USB-camera.mp4 ! qtdemux ! h264parse ! omxh264dec ! waylandsink
```