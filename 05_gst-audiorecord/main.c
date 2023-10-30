#include <gst/gst.h>

#define BITRATE      128000     /* Bitrate averaging */
#define SAMPLE_RATE  48000  /* Sample rate  of audio file*/
#define CHANNEL      1          /* Channel*/
#define OUTPUT_FILE  "RECORD_microphone-mono.ogg"
#define FORMAT       "F32LE"
#define ARG_PROGRAM_NAME   0
#define ARG_DEVICE         1
#define ARG_COUNT          2

typedef struct tag_user_data
{
  GstElement *pipeline;
  GstElement *source;
  GstElement *converter;
  GstElement *convert_capsfilter;
  GstElement *encoder;
  GstElement *muxer;
  GstElement *sink;

  const gchar *device;
} UserData;

static GstElement *pipeline;

void
signalHandler (int signal)
{
  if (signal == SIGINT) {
    gst_element_send_event (pipeline, gst_event_new_eos ());
  }
}

static void
link_to_multiplexer (GstPad * tolink_pad, GstElement * mux)
{
  GstPad *pad;
  gchar *srcname, *sinkname;

  srcname = gst_pad_get_name (tolink_pad);
  pad = gst_element_get_compatible_pad (mux, tolink_pad, NULL);
  gst_pad_link (tolink_pad, pad);
  sinkname = gst_pad_get_name (pad);
  gst_object_unref (GST_OBJECT (pad));

  g_print ("A new pad %s was created and linked to %s\n", sinkname, srcname);
  g_free (sinkname);
  g_free (srcname);
}

void
set_element_properties (UserData *data)
{
  GstCaps *caps;

  /* set input device (microphone) of the source element - alsasrc */
  g_object_set (G_OBJECT (data->source), "device", data->device, NULL);

  /* set target bitrate of the encoder element - vorbisenc */
  g_object_set (G_OBJECT (data->encoder), "bitrate", BITRATE, NULL);

  /* set output file location of the sink element - filesink */
  g_object_set (G_OBJECT (data->sink), "location", OUTPUT_FILE, NULL);

  /* create simple caps */
  caps =
      gst_caps_new_simple ("audio/x-raw",
      "format", G_TYPE_STRING, FORMAT,
      "channels", G_TYPE_INT, CHANNEL, "rate", G_TYPE_INT, SAMPLE_RATE, NULL);

  /* set caps property of capsfilters */
  g_object_set (G_OBJECT (data->convert_capsfilter), "caps", caps, NULL);
  gst_caps_unref (caps);
}

int
setup_pipeline (UserData *data)
{
  GstPad *srcpad;

  /* set element properties */
  set_element_properties (data);

  /* Add the elements into the pipeline and link them together */
  /* file-src -> converter -> vorbis-encoder -> oggmuxer -> file-output */
  gst_bin_add_many (GST_BIN (data->pipeline), data->source, data->converter,
      data->convert_capsfilter, data->encoder, data->muxer, data->sink, NULL);
  if (gst_element_link_many (data->source, data->converter,
          data->convert_capsfilter, data->encoder, NULL) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    return FALSE;
  }
  if (gst_element_link (data->muxer, data->sink) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    return FALSE;
  }

  /* link srcpad of vorbisenc to request pad of oggmux */
  srcpad = gst_element_get_static_pad (data->encoder, "src");
  link_to_multiplexer (srcpad, data->muxer);
  gst_object_unref (srcpad);

  return TRUE;
}

void
parse_message (GstMessage *msg)
{
  GError *error;
  gchar  *dbg_inf;

  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
      g_print ("End of stream !\n");
      break;
    case GST_MESSAGE_ERROR:
      gst_message_parse_error (msg, &error, &dbg_inf);
      g_printerr (" Element error %s: %s.\n",
          GST_OBJECT_NAME (msg->src), error->message);
      g_printerr ("Debugging information: %s.\n",
          dbg_inf ? dbg_inf : "none");
      g_clear_error (&error);
      g_free (dbg_inf);
      break;
    default:
      /* We don't care other message */
      g_printerr ("Undefined message.\n");
      break;
  }
}

int
main (int argc, char *argv[])
{
  GstBus *bus;
  GstMessage *msg;
  UserData user_data;

  if (argc != ARG_COUNT) {
    g_print ("Error: Invalid arugments.\n");
    g_print ("Usage: %s <microphone device> \n", argv[ARG_PROGRAM_NAME]);
    return -1;
  }

  /* Initialization */
  gst_init (&argc, &argv);

  /* Create GStreamer elements */
  user_data.pipeline = gst_pipeline_new ("audio-record");
  user_data.source = gst_element_factory_make ("alsasrc", "alsa-source");
  user_data.converter = gst_element_factory_make ("audioconvert",
                            "audio-converter");
  user_data.convert_capsfilter = gst_element_factory_make ("capsfilter",
                                     "convert_caps");
  user_data.encoder = gst_element_factory_make ("vorbisenc", "vorbis-encoder");
  user_data.muxer = gst_element_factory_make ("oggmux", "ogg-muxer");
  user_data.sink = gst_element_factory_make ("filesink", "file-output");

  if (!user_data.pipeline || !user_data.source || !user_data.converter ||
          !user_data.convert_capsfilter || !user_data.encoder ||
          !user_data.muxer || !user_data.sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  user_data.device = argv[ARG_DEVICE];

  if (setup_pipeline (&user_data) != TRUE) {
    gst_object_unref (user_data.pipeline);
    return -1;
  }

  /* Set the pipeline to "playing" state */
  g_print ("Now recording, press [Ctrl] + [C] to stop...\n");
  if (gst_element_set_state (user_data.pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (user_data.pipeline);
    return -1;
  }

  /* Handle signals gracefully. */
  pipeline = user_data.pipeline;
  signal (SIGINT, signalHandler);

  /* Wait until error or EOS */
  bus = gst_element_get_bus (user_data.pipeline);
  msg =
      gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
      GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
  gst_object_unref (bus);

  /* Note that because input timeout is GST_CLOCK_TIME_NONE,
     the gst_bus_timed_pop_filtered() function will block forever until a
     matching message was posted on the bus (GST_MESSAGE_ERROR or
     GST_MESSAGE_EOS). */

  if (msg != NULL) {
    parse_message (msg);
    gst_message_unref (msg);
  }

  g_print ("Returned, stopping recording...\n");
  gst_element_set_state (user_data.pipeline, GST_STATE_NULL);
  g_print ("Deleting pipeline...\n");
  gst_object_unref (GST_OBJECT (user_data.pipeline));
  g_print ("Succeeded. Please check output file: %s\n", OUTPUT_FILE);

  return 0;
}
