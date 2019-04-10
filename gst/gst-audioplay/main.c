#include <gst/gst.h>

#define INPUT_FILE  "/home/media/audios/Rondo_Alla_Turka.ogg"
#define FORMAT      "S16LE"

static void
on_pad_added (GstElement * element, GstPad * pad, gpointer data)
{
  GstPad *sinkpad;
  GstElement *decoder = (GstElement *) data;

  /* We can now link this pad with the vorbis-decoder sink pad */
  g_print ("Dynamic pad created, linking demuxer/decoder\n");

  sinkpad = gst_element_get_static_pad (decoder, "sink");

  gst_pad_link (pad, sinkpad);

  gst_object_unref (sinkpad);
}

int
main (int argc, char *argv[])
{
  GstElement *pipeline, *source, *demuxer, *decoder, *conv, *capsfilter, *sink;
  GstCaps *caps;
  GstBus *bus;
  GstMessage *msg;

  const gchar *input_file = INPUT_FILE;

  /* Initialization */
  gst_init (&argc, &argv);

  /* Create GStreamer elements */
  pipeline = gst_pipeline_new ("audio-play");
  source = gst_element_factory_make ("filesrc", "file-source");
  demuxer = gst_element_factory_make ("oggdemux", "ogg-demuxer");
  decoder = gst_element_factory_make ("vorbisdec", "vorbis-decoder");
  conv = gst_element_factory_make ("audioconvert", "converter");
  capsfilter = gst_element_factory_make ("capsfilter", "conv_capsfilter");
  sink = gst_element_factory_make ("autoaudiosink", "audio-output");

  if (!pipeline || !source || !demuxer || !decoder || !capsfilter || !conv
      || !sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  /* Set up the pipeline */

  /* Set the input filename to the source element */
  g_object_set (G_OBJECT (source), "location", input_file, NULL);

  /* Set the caps option to the caps-filter element */
  caps =
      gst_caps_new_simple ("audio/x-raw", "format", G_TYPE_STRING, FORMAT,
      NULL);
  g_object_set (G_OBJECT (capsfilter), "caps", caps, NULL);
  gst_caps_unref (caps);


  /* Add the elements into the pipeline */
  /* file-source | ogg-demuxer | vorbis-decoder | converter | alsa-output */
  gst_bin_add_many (GST_BIN (pipeline),
      source, demuxer, decoder, conv, capsfilter, sink, NULL);

  /* Link the elements together */
  /* file-source -> ogg-demuxer ~> vorbis-decoder -> converter -!-> alsa-output */
  if (gst_element_link (source, demuxer) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }
  if (gst_element_link_many (decoder, conv, capsfilter, sink, NULL) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  g_signal_connect (demuxer, "pad-added", G_CALLBACK (on_pad_added), decoder);

  /* note that the demuxer will be linked to the decoder dynamically.
     The reason is that Ogg may contain various streams (for example
     audio and video). The source pad(s) will be created at run time,
     by the demuxer when it detects the amount and nature of streams.
     Therefore we connect a callback function which will be executed
     when the "pad-added" is emitted. */

  /* Set the pipeline to "playing" state */
  g_print ("Now playing file %s:\n", input_file);
  if (gst_element_set_state (pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  /* Waiting */
  g_print ("Running...\n");
  bus = gst_element_get_bus (pipeline);
  msg =
      gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
      GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

  /* Note that because input timeout is GST_CLOCK_TIME_NONE, 
     the gst_bus_timed_pop_filtered() function will block forever untill a 
     matching message was posted on the bus (GST_MESSAGE_ERROR or 
     GST_MESSAGE_EOS). */

  /* Playback end. Clean up nicely */
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

  gst_object_unref (bus);
  g_print ("Returned, stopping playback...\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);
  g_print ("Deleting pipeline...\n");
  gst_object_unref (GST_OBJECT (pipeline));
  g_print ("Program end!\n");
  return 0;
}
