# Audio Record

Record raw data from USB microphone, then store it in Ogg container.

![Figure audio record pipeline](figure.png)

## Development Environment

GStreamer: 1.16.3 (edited by Renesas).

## Application Content

+ [`main.c`](main.c)
+ [`Makefile`](Makefile)

### Walkthrough: [`main.c`](main.c)
>Note that this tutorial only discusses the important points of this application. For the rest of source code, please refer to section [Audio Play](/01_gst-audioplay/README.md).
#### Output location
```c
#define OUTPUT_FILE  "RECORD_microphone-mono.ogg"
const gchar *output_file = OUTPUT_FILE;
```
#### Command-line argument
```c
if (argc != ARG_COUNT) {
  g_print ("Error: Invalid arugments.\n");
  g_print ("Usage: %s <microphone device> \n", argv[ARG_PROGRAM_NAME]);
  return -1;
}
```
This application accepts a command-line argument which points to a microphone (hw:1,0, for example).

>Note: You can find this value by following section [Special Instruction](#special-instruction)

#### Create elements
```c
pipeline = gst_pipeline_new ("audio-record");
source = gst_element_factory_make ("alsasrc", "alsa-source");
converter = gst_element_factory_make ("audioconvert", "audio-converter");
convert_capsfilter = gst_element_factory_make ("capsfilter", "convert_caps");
encoder = gst_element_factory_make ("vorbisenc", "vorbis-encoder");
muxer = gst_element_factory_make ("oggmux", "ogg-muxer");
sink = gst_element_factory_make ("filesink", "file-output");
```
To record raw data from microphone then store it in Ogg container, the following elements are used:
-	 Element `alsasrc` reads data from an audio card using the ALSA API.
-	 Element `audioconvert` converts raw audio buffers to a format (such as: F32LE) which is understood by vorbisenc.
-	 Element `vorbisenc` encodes raw audio into a Vorbis stream.
-	 Element `oggmux` merges audio stream to Ogg container.
-	 Element `filesink` writes incoming data to a local file.

#### Set element’s properties
```c
g_object_set (G_OBJECT (source), "device", argv[ARG_DEVICE], NULL);
g_object_set (G_OBJECT (encoder), "bitrate", BITRATE, NULL);
g_object_set (G_OBJECT (sink), "location", output_file, NULL);
```
The `g_object_set()` function is used to set some element’s properties, such as:
-	 The `device` property of alsasrc element which points to a microphone device. Users will pass the device card as a command line argument to this application. Please refer to section [Special Instruction](#special-instruction) to find the value.
-	 The `location` property of filesink element which points to the output file.
-	 The `bitrate` property of vorbisenc element is used to specify encoding bit rate. The higher bitrate, the better quality.

```c
caps = gst_caps_new_simple ("audio/x-raw", "format", G_TYPE_STRING, FORMAT,
           "channels", G_TYPE_INT, CHANNEL, "rate", G_TYPE_INT, SAMPLE_RATE, NULL);
g_object_set (G_OBJECT (convert_capsfilter), "caps", caps, NULL);
gst_caps_unref (caps);
```
Capabilities (short: `caps`) describe the type of data which is streamed between two pads. This data includes raw audio format, channel, and sample rate.\
The `gst_caps_new_simple()` function creates new caps which holds these values. These caps are then added to `caps` property of capsfilter elements `(g_object_set)`.
>Note that both caps should be freed with `gst_caps_unref()` if they are not used anymore.

#### Build pipeline
```c
gst_bin_add_many (GST_BIN (pipeline), source, converter, convert_capsfilter,
    encoder, muxer, sink, NULL);
gst_element_link_many (source, converter, convert_capsfilter, encoder, NULL);
gst_element_link (muxer, sink);
```
The reason for the separation is that the sink pad of oggmux `(muxer)` cannot be created automatically but is only created on demand. This application uses self-defined function `link_to_multiplexer()` to link the sink pad to source pad of vorbisenc `(encoder)`. That’s why its sink pad is called Request Pad.
>Note that the order counts, because links must follow the data flow (this is, from source elements to sink elements).

#### Link request pads

```c
srcpad = gst_element_get_static_pad (encoder, "src");
link_to_multiplexer (srcpad, muxer);
gst_object_unref (srcpad);
```
This block gets the source pad `(srcpad)` of 1, then calls `link_to_multiplexer()` to link it to (the sink pad of) oggmux `(muxer)`.
>Note that the srcpad should be freed with `gst_object_unref()` if it is not used anymore.

```c
static void link_to_multiplexer (GstPad * tolink_pad, GstElement * mux)
{
  pad = gst_element_get_compatible_pad (mux, tolink_pad, NULL);
  gst_pad_link (tolink_pad, pad);

  gst_object_unref (GST_OBJECT (pad));
}
```
This function uses `gst_element_get_compatible_pad()` to request a sink pad `(pad)` which is compatible with the source pad `(tolink_pad)` of oggmux `(mux)`, then calls `gst_pad_link()` to link them together.
>Note that the pad should be freed with `gst_object_unref()` if it is not used anymore.

### Play pipeline
```c
gst_element_set_state (pipeline, GST_STATE_PLAYING);
```

Every pipeline has an associated [state](https://gstreamer.freedesktop.org/documentation/plugin-development/basics/states.html). To start audio recording, the `pipeline` needs to be set to PLAYING state.

```c
signal (SIGINT, signalHandler);
```
This application will stop recording if user presses Ctrl-C. To do so, it uses `signal()` to bind `SIGINT` (interrupt from keyboard) to `signalHandler()`.
To know how this function is implemented, please refer to the following lines of code:

```c
void signalHandler (int signal)
{
  if (signal == SIGINT) {
    gst_element_send_event (pipeline, gst_event_new_eos ());
  }
}
```
It calls `gst_element_send_event()` to send EOS (End-of-Stream) signal `(gst_event_new_eos)` to the `pipeline`. This makes `gst_bus_timed_pop_filtered()` return. Finally, the program cleans up GStreamer objects and exits.

## How to Build and Run GStreamer Application

This section shows how to cross-compile and deploy GStreamer _audio record_ application.

### How to Extract Renesas SDK
Please refer to _hello word_ [How to Extract Renesas SDK section](/00_gst-helloworld/README.md#how-to-extract-renesas-sdk) for more details.

### How to Build and Run GStreamer Application

***Step 1***.	Go to gst-audiorecord directory:
```sh
$   cd $WORK/05_gst-audiorecord
```

***Step 2***.	Cross-compile:
```sh
$   make
```
***Step 3***.	Copy all files inside this directory to _/usr/share_ directory on the target board:
```sh
$   scp -r $WORK/05_gst-audiorecord/ <username>@<board IP>:/usr/share/
```
***Step 4***.	Run the application:

```sh
$   /usr/share/05_gst-audiorecord/gst-audiorecord $(/usr/share/05_gst-audiorecord/detect_microphone.sh)
```
For more details about _detect_microphone.sh_ script at [Special instruction](#special-instruction)
### Special instruction:
#### Run the following script to find microphone device card:
```sh
$ ./detect_microphone.sh
```
Basically, this script analyzes the `/proc/asound/cards` file to get sound cards.
>Note: This script can be used in combination with gst-audiorecord application.

For further information on how this script is implemented, please refer to the following lines of code:
```sh
ALSA_DEV_FILE="/proc/asound/cards"

CMD_GET_SND="cat $ALSA_DEV_FILE | awk '{ print \$0 }'"

CMD_GET_SND_INDICES="$CMD_GET_SND | awk '{ print \$1 }'"

SND_INDICES="$( eval $CMD_GET_SND_INDICES )"
if [ ! -z "$SND_INDICES" ]
then
    for TEMP_INDEX in $SND_INDICES
    do
        HAS_MIC="$( amixer -D hw:$TEMP_INDEX scontrols | grep "Mic" )"

        if [ "$HAS_MIC" != "" ]
        then
            SND_INDEX=$TEMP_INDEX
            DEVICE_NUMBER=${HAS_MIC: -1}
            echo "hw:$SND_INDEX,$DEVICE_NUMBER"

	    amixer cset name='Left Input Mixer L2 Switch' on > /dev/null
	    amixer cset name='Right Input Mixer R2 Switch' on > /dev/null
	    amixer cset name='Headphone Playback Volume' 100 > /dev/null
	    amixer cset name='PCM Volume' 100% > /dev/null
	    amixer cset name='Input PGA Volume' 25 > /dev/null
            break
        fi
    done
fi
```
#### To check the output file:
Option 1: VLC media player (https://www.videolan.org/vlc/index.html).
Option 2: Tool gst-launch-1.0 (on board):
```sh
$ gst-launch-1.0 filesrc location=RECORD_microphone-mono.ogg ! oggdemux ! vorbisdec ! audioconvert ! audioresample ! autoaudiosink
```
