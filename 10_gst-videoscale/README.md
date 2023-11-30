# Video Scale

Scale down an H.264 video, then store it in MP4 container.

![Figure video scale pipeline](figure.png)

## Development Environment

GStreamer: 1.16.3 (edited by Renesas).

## Application Content

+ [`main.c`](main.c)
+ [`Makefile`](Makefile)

### Walkthrough: [`main.c`](main.c)
>Note that this tutorial only discusses the important points of this application. For the rest of source code, please refer to section [Video Record](../06_gst-videorecord/README.md) and [Audio Play](/01_gst-audioplay/README.md).

#### UserData structure
```c
typedef struct tag_user_data
{
  GstElement *pipeline;
  GstElement *source;
  GstElement *demuxer;
  GstElement *parser1;
  GstElement *decoder;
  GstElement *filter;
  GstElement *capsfilter;
  GstElement *encoder;
  GstElement *parser2;
  GstElement *muxer;
  GstElement *sink;

  const gchar *input_file;
  int scaled_width;
  int scaled_height;
} UserData;
```
This structure contains:
- Gstreamer element variables: `pipeline`, `source`, `demuxer`, `parser1`, `decoder`, `filter`, `capsfilter`, `encoder`, `parser2`, `muxer`, `sink`. These variables will be used to create pipeline and elements as section [Create elements](#create-elements).
- Variable `input_file (const gchar)` to represent MP4 video input file.
- Variable `scaled_width (int)` and `scaled_height (int)` are width and height of video after scale down.

#### Command-line argument
```c
if (argc != ARG_COUNT)
{
  g_print ("Invalid arugments.\n");
  g_print ("Usage: %s <MP4 file> <width> <>height \n", argv[ARG_PROGRAM_NAME]);
  return -1;
}
```
This application accepts two command-line arguments as below:
-	 width/height: output resolution.
-	 An MP4 file’s location.

#### Create elements
```c
user_data.source = gst_element_factory_make ("filesrc", "video-src");
user_data.demuxer = gst_element_factory_make ("qtdemux", "mp4-demuxer");
user_data.parser1 = gst_element_factory_make ("h264parse", "h264-parser-1");
user_data.decoder = gst_element_factory_make ("omxh264dec", "video-decoder");
user_data.filter = gst_element_factory_make ("vspmfilter", "video-filter");
user_data.capsfilter = gst_element_factory_make ("capsfilter", "capsfilter");
user_data.encoder = gst_element_factory_make ("omxh264enc", "video-encoder");
user_data.parser2 = gst_element_factory_make ("h264parse", "h264-parser-2");
user_data.muxer = gst_element_factory_make ("qtmux", "mp4-muxer");
user_data.sink = gst_element_factory_make ("filesink", "file-output");
```
To scale down an H.264 video and store it in MP4 container, the following elements are needed:
-	 Element `filesrc` reads data from a local file.
-	 Element `qtdemux` de-multiplexes an MP4 file into audio and video stream.
-	 Element `omxh264dec` decompresses H.264 stream to raw NV12-formatted video.
-	 Element `vspmfilter` handles video scaling.
-	 Element `capsfilter` contains resolution so that vspfilter will scale video frames based on this value.
-	 Element `omxh264enc` encodes raw video into H.264 compressed data.
-	 Element `h264parse` parses H.264 video from byte stream format to AVC format which `omxh264dec` can process.
-	 Element `qtmux` merges H.264 byte stream to MP4 container.
-	 Element `filesink` writes incoming data to a local file.

#### Set element’s properties
```c
g_object_set (G_OBJECT (data->source), "location", data->input_file, NULL);
g_object_set (G_OBJECT (data->filter), "dmabuf-use", TRUE, NULL);
g_object_set (G_OBJECT (data->encoder), "target-bitrate", BITRATE_OMXH264ENC,
    "control-rate", 1, NULL);
g_object_set (G_OBJECT (data->sink), "location", OUTPUT_FILE, NULL);
```
The `g_object_set()` function is used to set some element’s properties, such as:
-	 The `location` property of filesrc element which points to an MP4 input file.
-	 The `dmabuf-use` property of vspmfilter element, This disallows dmabuf to be output buffer. If it is not set, waylandsink will display broken video frames.
-	 The `target-bitrate` property of omxh264enc element which is set to 40 Mbps. The higher bitrate, the better quality.
-	 The `control-rate` property of omxh264enc element is used to specify birate control method which is variable bitrate method in this case.
-	 The `location` property of filesink element which points to MP4 output file.
```c
scale_caps =
    gst_caps_new_simple ("video/x-raw", "width", G_TYPE_INT, scaled_width, "height",
    G_TYPE_INT, scaled_height, NULL);

g_object_set (G_OBJECT (puser_data->capsfilter), "caps", scale_caps, NULL);
gst_caps_unref (scale_caps);
```
Capabilities (short: `caps`) describe the type of data which is streamed between two pads. This data includes raw video format, resolution, and framerate.\
The `gst_caps_new_simple()` function creates a new cap (scale_caps) which holds output’s resolution. This cap is then added to caps property of capsfilter `(g_object_set)` so that vspfilter will use these values to resize video frames.

>Note that the `scale_caps` should be freed with `gst_caps_unref()` if it is not used anymore.
#### Get input file’s information
```c
new_pad_caps = gst_pad_query_caps (pad, NULL);
new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);

gst_structure_get_int (new_pad_struct, "width", &width);
gst_structure_get_int (new_pad_struct, "height", &height);
```
Above lines of code get the capabilities of pad, finds the structure in `new_pad_caps` then gets resolution of video.

## How to Build and Run GStreamer Application

This section shows how to cross-compile and deploy GStreamer _video scale_ application.

### How to Extract Renesas SDK
***Step 1***.	Install toolchain on a Host PC:
```sh
$   sudo sh ./poky-glibc-x86_64-core-image-weston-aarch64-smarc-rzg2l-toolchain-3.1.17.sh
```
Note:
> This step installs the RZG2L toolchain. If you want to install the RZV2L toolchain, please use `poky-glibc-x86_64-core-image-weston-aarch64-smarc-rzv2l-toolchain-3.1.17.sh` instead.\
> Sudo is optional in case user wants to extract SDK into a restricted directory (such as: _/opt/_)

If the installation is successful, the following messages will appear:
```sh
SDK has been successfully set up and is ready to be used.
Each time you wish to use the SDK in a new shell session, you need to source the environment setup script e.g.
$ . /opt/poky/3.1.17/environment-setup-aarch64-poky-linux
$ . /opt/poky/3.1.17/environment-setup-armv7vet2hf-neon-vfpv4-pokymllib32-linux-gnueabi
```
***Step 2***.	Set up cross-compile environment:
```sh
$   source /<Location in which SDK is extracted>/environment-setup-aarch64-poky-linux
```
Note:
>User needs to run the above command once for each login session.

### How to Build and Run GStreamer Application

***Step 1***.	Go to gst-videoscale directory:
```sh
$   cd $WORK/10_gst-videoscale
```

***Step 2***.	Cross-compile:
```sh
$   make
```
***Step 3***.	Copy all files inside this directory to _/usr/share_ directory on the target board:
```sh
$   scp -r $WORK/10_gst-videoscale/ <username>@<board IP>:/usr/share/
```
***Step 4***.	Run the application:
> RZG2L and RZV2L do not support scale up.
```sh
$   /usr/share/10_gst-videoscale/gst-videoscale <MP4 file> <width> <height>
```
- In this case, a 640x360 MP4 file will be generated after running this application.

  Download the input file `sintel_trailer-720p.mp4` as described in _Sintel_trailer/README.md_ file in media repository [(github.com/renesas-rz/media)](https://github.com/renesas-rz/media) and then place it in _/home/media/videos_.\
  ```sh
  $   /usr/share/10_gst-videoscale/gst-videoscale /home/media/videos/sintel_trailer-720p.mp4 640 360
  ```
  >Note that output does not have audio. If it's resolution is abnormal, this application will not work.