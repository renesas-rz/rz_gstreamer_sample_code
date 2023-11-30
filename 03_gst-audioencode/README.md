# Audio Encode

Encode audio data from F32LE raw format to Ogg/Vorbis format.

![Figure audio encode pipeline](figure.png)

## Development Environment

GStreamer: 1.16.3 (edited by Renesas).

## Application Content

+ [`main.c`](main.c)
+ [`Makefile`](Makefile)

### Walkthrough: [`main.c`](main.c)
>Note that this tutorial only discusses the important points of this application. For the rest of source code, please refer to section [Audio Play](../01_gst-audioplay/README.md).

#### UserData structure
```c
typedef struct tag_user_data
{
  GstElement *pipeline;
  GstElement *source;
  GstElement *capsfilter;
  GstElement *encoder;
  GstElement *parser;
  GstElement *muxer;
  GstElement *sink;
} UserData;
```
This structure contains:
- Gstreamer element variables: `pipeline`, `source`, `capsfilter`, `encoder`, `parser`, `muxer`, `sink`. These variables will be used to create pipeline and elements as section [Create elements](#create-elements).

#### Input/output location
```c
#define INPUT_FILE       "/home/media/audios/Rondo_Alla_Turka_F32LE_44100_stereo.raw"
#define OUTPUT_FILE      "/home/media/audios/ENCODE_Rondo_Alla_Turka.ogg"
```
Note:
> You can create input file by following section [Special Instruction](#special-instruction)
#### Create elements
```c
user_data.source = gst_element_factory_make ("filesrc", "file-source");
user_data.capsfilter = gst_element_factory_make ("capsfilter",
                            "caps-filter");
user_data.encoder = gst_element_factory_make ("vorbisenc", "vorbis-encoder");
user_data.parser = gst_element_factory_make ("vorbisparse", "vorbis-parser");
user_data.muxer = gst_element_factory_make ("oggmux", "OGG-muxer");
user_data.sink = gst_element_factory_make ("filesink", "file-output");
```
To encode an audio file to Vorbis format, the following elements are used:
-	 Element `filesrc` reads data from a local file.
-	 Element `capsfilter` specifies raw audio format, channel, and bitrate.
-	 Element `vorbisenc` encodes raw float audio into a Vorbis stream.
-	 Element `vorbisparse` parses the header packets of the Vorbis stream and put them as the stream header in the caps.
-	 Element `oggmux` merges streams (audio and/or video) into Ogg files. In this case, only audio stream is available.
-	 Element `filesink` writes incoming data to a local file.

#### Set element’s properties
```c
  g_object_set (G_OBJECT (data->encoder), "bitrate", BITRATE, NULL);
  g_object_set (G_OBJECT (data->source), "location", INPUT_FILE, NULL);
  g_object_set (G_OBJECT (data->sink), "location", OUTPUT_FILE, NULL);
```
The `g_object_set()` function is used to set some element’s properties, such as:
-	 The `bitrate` property of vorbisenc element which is set to 128 Kbps. Note that this value is just for demonstration purpose only. Users can define other value which will affect output quality.
-	 The `location` property of filesrc element which points to a raw audio file.
-	 The `location` property of filesink element which points to an output file.
```c
caps =
    gst_caps_new_simple ("audio/x-raw",
    "format", G_TYPE_STRING, FORMAT,
    "rate", G_TYPE_INT, SAMPLE_RATE, "channels", G_TYPE_INT, CHANNEL, NULL);
g_object_set (G_OBJECT (data->capsfilter), "caps", caps, NULL);

gst_caps_unref (caps);
```
A `capsfilter` is needed between filesrc and vorbisenc because the vorbisenc element needs to know what raw audio format, sample rate, and channels of the incoming data stream are. In this application, audio file is formatted to F32LE, has sample rate 44.1 kHz and stereo audio.\
The `gst_caps_new_simple()` function creates a new cap which holds these values. This cap is then added to caps property of capsfilter element `(g_object_set)`.
>Note that the caps should be freed with `gst_caps_unref()` if it is not used anymore.

## How to Build and Run GStreamer Application

This section shows how to cross-compile and deploy GStreamer _audio encode_ application.

### How to Extract Renesas SDK
***Step 1***. Install toolchain on a Host PC:
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
***Step 2***. Set up cross-compile environment:
```sh
$   source /<Location in which SDK is extracted>/environment-setup-aarch64-poky-linux
```
Note:
>User needs to run the above command once for each login session.

### How to Build and Run GStreamer Application

***Step 1***. Go to gst-audioencode directory:
```sh
$   cd $WORK/03_gst-audioencode
```
***Step 2***. Cross-compile:
```sh
$   make
```
***Step 3***. Copy all files inside this directory to _/usr/share_ directory on the target board:
```sh
$   scp -r $WORK/03_gst-audioencode/ <username>@<board IP>:/usr/share/
```
***Step 4***. Run the application:
```sh
$   /usr/share/03_gst-audioencode/gst-audioencode
```
### Special instruction:
#### Prepare raw audio file:
1. Download the input file `Rondo_Alla_Turka.ogg` from _Renesas/audios_ in media repository [(github.com/renesas-rz/media)](https://github.com/renesas-rz/media) and then place it in _/home/media/audios_.
2. Run this command (on board) to convert this file to raw audio format (F32LE, 44.1 kHz, and stereo audio).
```sh
$ gst-launch-1.0 -e filesrc location=/home/media/audios/Rondo_Alla_Turka.ogg ! oggdemux ! vorbisdec ! audio/x-raw, format=F32LE, rate=44100, channels=2 ! filesink location=/home/media/audios/Rondo_Alla_Turka_F32LE_44100_stereo.raw
```
>Note that this process will take a while depending on the file size and processor speed.
#### To check the output file:

Option 1: VLC media player (https://www.videolan.org/vlc/index.html).

Option 2: Tool gst-launch-1.0 (on board):
```sh
$ gst-launch-1.0 filesrc location=/home/media/audios/ENCODE_Rondo_Alla_Turka.ogg ! oggdemux ! vorbisdec ! audioconvert ! audio/x-raw, format=S16LE ! alsasink
```