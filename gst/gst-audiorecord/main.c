#include <gst/gst.h>

#define BITRATE      128000     /* Bitrate averaging */
#define SAMPLE_RATE  44100  /* Sample rate  of audio file*/
#define CHANNEL      1          /* Channel*/
#define OUTPUT_FILE  "/home/media/audios/RECORD_microphone-mono.ogg"
#define FORMAT       "F32LE"

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

int
main (int argc, char *argv[])
{
  GstElement *source, *converter, *convert_capsfilter, *encoder, *muxer, *sink;
  GstCaps *caps;
  GstBus *bus;
  GstPad *srcpad;
  GstMessage *msg;

  const gchar *output_file = OUTPUT_FILE;

  /* Initialization */
  gst_init (&argc, &argv);

  /* Create GStreamer elements */
  pipeline = gst_pipeline_new ("audio-record");
  source = gst_element_factory_make ("alsasrc", "alsa-source");
  converter = gst_element_factory_make ("audioconvert", "audio-converter");
  convert_capsfilter = gst_element_factory_make ("capsfilter", "convert_caps");
  encoder = gst_element_factory_make ("vorbisenc", "vorbis-encoder");
  muxer = gst_element_factory_make ("oggmux", "ogg-muxer");
  sink = gst_element_factory_make ("filesink", "file-output");

  if (!pipeline || !source || !converter || !convert_capsfilter || !encoder
      || !muxer || !sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  /* set input device (microphone) of the source element - alsasrc */
  g_object_set (G_OBJECT (source), "device", "hw:0,0", NULL);

  /* set target bitrate of the encoder element - vorbisenc */
  g_object_set (G_OBJECT (encoder), "bitrate", BITRATE, NULL);

  /* set output file location of the sink element - filesink */
  g_object_set (G_OBJECT (sink), "location", output_file, NULL);

  /* create simple caps */
  caps =
      gst_caps_new_simple ("audio/x-raw",
      "format", G_TYPE_STRING, FORMAT,
      "channels", G_TYPE_INT, CHANNEL, "rate", G_TYPE_INT, SAMPLE_RATE, NULL);

  /* set caps property of capsfilters */
  g_object_set (G_OBJECT (convert_capsfilter), "caps", caps, NULL);
  gst_caps_unref (caps);

  /* add the elements into the pipeline */
  gst_bin_add_many (GST_BIN (pipeline), source, converter, convert_capsfilter,
      encoder, muxer, sink, NULL);

  /* link the elements together */
  if (gst_element_link_many (source, converter, convert_capsfilter, encoder,
          NULL) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }
  if (gst_element_link (muxer, sink) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  /* link srcpad of vorbisenc to request pad of oggmux */
  srcpad = gst_element_get_static_pad (encoder, "src");
  link_to_multiplexer (srcpad, muxer);
  gst_object_unref (srcpad);

  /* Set the pipeline to "playing" state */
  g_print ("Now recording, press [Ctrl] + [C] to stop...\n");
  if (gst_element_set_state (pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  /* Handle signals gracefully. */
  signal (SIGINT, signalHandler);

  /* Wait until error or EOS */
  bus = gst_element_get_bus (pipeline);
  msg =
      gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
      GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
  gst_object_unref (bus);

  /* Note that because input timeout is GST_CLOCK_TIME_NONE,
     the gst_bus_timed_pop_filtered() function will block forever until a
     matching message was posted on the bus (GST_MESSAGE_ERROR or
     GST_MESSAGE_EOS). */

  if (msg != NULL) {
    GError *err;
    gchar *debug_info;

    switch (GST_MESSAGE_TYPE (msg)) {
      case GST_MESSAGE_ERROR:
        gst_message_parse_error (msg, &err, &debug_info);
        g_printerr ("Error received from element %s: %s.\n",
            GST_OBJECT_NAME (msg->src), err->message);
        g_printerr ("Debugging information: %s.\n",
            debug_info ? debug_info : "none");
        g_clear_error (&err);
        g_free (debug_info);
        break;
      case GST_MESSAGE_EOS:
        g_print ("End-Of-Stream reached.\n");
        break;
      default:
        /* We should not reach here because we only asked for ERRORs and EOS */
        g_printerr ("Unexpected message received.\n");
        break;
    }
    gst_message_unref (msg);
  }

  /* Clean up nicely */
  g_print ("Returned, stopping recording...\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_print ("Deleting pipeline...\n");
  gst_object_unref (GST_OBJECT (pipeline));
  g_print ("Succeeded. Recorded file available at: %s\n", output_file);
  return 0;
}
