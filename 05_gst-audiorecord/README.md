# Audio Record

Record raw data from USB microphone, then store it in Ogg container.

![Figure audio record pipeline](figure.png)

## Development Environment

GStreamer: 1.16.3 (edited by Renesas).

## Application Content

+ [`main.c`](main.c)

### Walkthrought
>Note that this tutorial only discusses the important points of this application. For the rest of source code, please refer to section [Audio Play](../01_gst-audioplay/README.md).
#### Output location
```
#define OUTPUT_FILE  "RECORD_microphone-mono.ogg"
const gchar *output_file = OUTPUT_FILE;
```
#### Command-line argument
```
if (argc != ARG_COUNT) {
  g_print ("Error: Invalid arugments.\n");
  g_print ("Usage: %s <microphone device> \n", argv[ARG_PROGRAM_NAME]);
  return -1;
}
```
This application accepts a command-line argument which points to a microphone (hw:1,0, for example).

>Note: You can find this value by following section [Special Instruction](#special-instruction)

#### Create elements
```
pipeline = gst_pipeline_new ("audio-record");
source = gst_element_factory_make ("alsasrc", "alsa-source");
converter = gst_element_factory_make ("audioconvert", "audio-converter");
convert_capsfilter = gst_element_factory_make ("capsfilter", "convert_caps");
encoder = gst_element_factory_make ("vorbisenc", "vorbis-encoder");
muxer = gst_element_factory_make ("oggmux", "ogg-muxer");
sink = gst_element_factory_make ("filesink", "file-output");
```
To record raw data from microphone then store it in Ogg container, the following elements are used:
-	 Element alsasrc reads data from an audio card using the ALSA API.
-	 Element audioconvert converts raw audio buffers to a format (such as: F32LE) which is understood by vorbisenc.
-	 Element vorbisenc encodes raw audio into a Vorbis stream.
-	 Element oggmux merges audio stream to Ogg container.
-	 Element filesink writes incoming data to a local file.

#### Set element’s properties
```
g_object_set (G_OBJECT (source), "device", argv[ARG_DEVICE], NULL);
g_object_set (G_OBJECT (encoder), "bitrate", BITRATE, NULL);
g_object_set (G_OBJECT (sink), "location", output_file, NULL);
```
The _g_object_set()_ function is used to set some element’s properties, such as:
-	 The device property of alsasrc element which points to a microphone device. Users will pass the device card as a command line argument to this application. Please refer to section [Special Instruction](#special-instruction) to find the value.
-	 The location property of filesink element which points to the output file.
-	 The bitrate property of vorbisenc element is used to specify encoding bit rate. The higher bitrate, the better quality.

```
caps = gst_caps_new_simple ("audio/x-raw", "format", G_TYPE_STRING, FORMAT,
           "channels", G_TYPE_INT, CHANNEL, "rate", G_TYPE_INT, SAMPLE_RATE, NULL);
g_object_set (G_OBJECT (convert_capsfilter), "caps", caps, NULL);
gst_caps_unref (caps);
```
Capabilities (short: caps) describe the type of data which is streamed between two pads. This data includes raw audio format, channel, and sample rate.
The _gst_caps_new_simple()_ function creates new caps which holds these values. These caps are then added to caps property of capsfilter elements (g_object_set).
Note that both caps should be freed with _gst_caps_unref()_ if they are not used anymore.

#### Build pipeline
```
gst_bin_add_many (GST_BIN (pipeline), source, converter, convert_capsfilter,
    encoder, muxer, sink, NULL);
gst_element_link_many (source, converter, convert_capsfilter, encoder, NULL);
gst_element_link (muxer, sink);
```
The reason for the separation is that the sink pad of oggmux (muxer) cannot be created automatically but is only created on demand. This application uses self-defined function _link_to_multiplexer()_ to link the sink pad to source pad of vorbisenc (encoder). That’s why its sink pad is called Request Pad.
Note that the order counts, because links must follow the data flow (this is, from source elements to sink elements).

#### Link request pads

```
srcpad = gst_element_get_static_pad (encoder, "src");
link_to_multiplexer (srcpad, muxer);
gst_object_unref (srcpad);
```
This block gets the source pad (srcpad) of vorbisenc (encoder), then calls _link_to_multiplexer()_ to link it to (the sink pad of) oggmux (muxer).
Note that the srcpad should be freed with _gst_object_unref()_ if it is not used anymore.

```
static void link_to_multiplexer (GstPad * tolink_pad, GstElement * mux)
{
  pad = gst_element_get_compatible_pad (mux, tolink_pad, NULL);
  gst_pad_link (tolink_pad, pad);

  gst_object_unref (GST_OBJECT (pad));
}
```
This function uses _gst_element_get_compatible_pad()_ to request a sink pad (pad) which is compatible with the source pad (tolink_pad) of oggmux (mux), then calls _gst_pad_link()_ to link them together.
Note that the pad should be freed with _gst_object_unref()_ if it is not used anymore.

### Play pipeline
```
gst_element_set_state (pipeline, GST_STATE_PLAYING);
```

Every pipeline has an associated [state](https://gstreamer.freedesktop.org/documentation/plugin-development/basics/states.html). To start audio recording, the pipeline needs to be set to PLAYING state.

```
signal (SIGINT, signalHandler);
```
This application will stop recording if user presses Ctrl-C. To do so, it uses _signal()_ to bind SIGINT (interrupt from keyboard) to _signalHandler()_.
To know how this function is implemented, please refer to the following code block:

```
void signalHandler (int signal)
{
  if (signal == SIGINT) {
    gst_element_send_event (pipeline, gst_event_new_eos ());
  }
}
```
It calls _gst_element_send_event()_ to send EOS (End-of-Stream) signal (gst_event_new_eos) to the pipeline. This makes _gst_bus_timed_pop_filtered()_ return. Finally, the program cleans up GStreamer objects and exits.

## How to Build and Run GStreamer Application

This section shows how to cross-compile and deploy GStreamer _audio record_ application.

### How to Extract SDK
Please refer to _hello word_ [README.md](/00_gst-helloworld/README.md) for more details.

### How to Build and Run GStreamer Application

***Step 1***.	Go to gst-audiorecord directory:
```
$   cd $WORK/05_gst-audiorecord
```

***Step 2***.	Cross-compile:
```
$   make
```
***Step 3***.	Copy all files inside this directory to /usr/share directory on the target board:
```
$   scp -r $WORK/05_gst-audiorecord/ <username>@<board IP>:/usr/share/
```
***Step 4***.	Run the application:

```
$   /usr/share/05_gst-audiorecord/gst-audiorecord $(/usr/share/05_gst-audiorecord/detect_microphone.sh)
```
For more details about _detect_microphone.sh_ script at [Special instruction](#special-instruction)
### Special instruction:
#### Run the following script to find microphone device card:
```
$ ./detect_microphone.sh
```
Basically, this script analyzes the /proc/asound/cards file to get sound cards.
>Note: This script can be used in combination with gst-audiorecord application.

For further information on how this script is implemented, please refer to the following code block:
```
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
```
$ gst-launch-1.0 filesrc location=RECORD_microphone-mono.ogg ! oggdemux ! vorbisdec ! audioconvert ! audioresample ! autoaudiosink
```
