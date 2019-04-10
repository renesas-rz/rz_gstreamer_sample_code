#include <gst/gst.h>

#define INPUT_FILE    "/home/media/videos/sintel_trailer-720p.mp4"
#define PORT          5000
#define PAYLOAD_TYPE  96
#define TIME          3  /* Send SPS and PPS Insertion every 3 second */

static void
on_pad_added (GstElement * element, GstPad * pad, gpointer data)
{
  GstPad *sinkpad;
  GstElement *decoder = (GstElement *) data;

  /* We can now link this pad with the H.264-decoder sink pad */
  g_print ("Dynamic pad created, linking demuxer/decoder\n");

  sinkpad = gst_element_get_static_pad (decoder, "sink");

  gst_pad_link (pad, sinkpad);

  gst_object_unref (sinkpad);
}

int
main (int argc, char *argv[])
{
  GstElement *pipeline, *source, *demuxer, *parser, *parser_capsfilter,
      *payloader, *sink;
  GstBus *bus;
  GstMessage *msg;
  GstCaps *parser_caps;

  const gchar *input_file = INPUT_FILE;

  if (argv[1] == NULL) {
    g_print ("No input! Please insert the streaming IP for this app\n");
    return -1;
  }

  /* Initialization */
  gst_init (&argc, &argv);

  /* Create GStreamer elements */
  pipeline = gst_pipeline_new ("send-streaming");
  source = gst_element_factory_make ("filesrc", "file-src");
  demuxer = gst_element_factory_make ("qtdemux", "mp4-demuxer");
  parser = gst_element_factory_make ("h264parse", "h264-parser");
  parser_capsfilter =
      gst_element_factory_make ("capsfilter", "parser-capsfilter");
  payloader = gst_element_factory_make ("rtph264pay", "h264-payloader");
  sink = gst_element_factory_make ("udpsink", "stream-output");

  if (!pipeline || !source || !demuxer || !parser || !parser_capsfilter
      || !payloader || !sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  /* Set input video device file of the source element - v4l2src */
  g_object_set (G_OBJECT (source), "location", input_file, NULL);

  /* Send SPS and PPS Insertion every 3 second.
       https://www.quora.com/What-are-SPS-and-PPS-in-video-codecs
     For H.264 AVC, dynamic payload is used.
       https://en.wikipedia.org/wiki/RTP_audio_video_profile
       http://www.freesoft.org/CIE/RFC/1890/29.htm */
  /* The stream is H.264 AVC so that the payload type must be a dynamic type (96-127).
     The value 96 is used as a default value */
  g_object_set (G_OBJECT (payloader), "pt", PAYLOAD_TYPE, "config-interval", TIME, NULL); 

  /* Listening to port 5000 of the host */
  g_object_set (G_OBJECT (sink), "host", argv[1], "port", PORT, NULL);

  /* create simple caps */
  parser_caps =
      gst_caps_new_simple ("video/x-h264", "stream-format", G_TYPE_STRING,
      "avc", "alignment", G_TYPE_STRING, "au", NULL);

  /* set caps property for capsfilters */
  g_object_set (G_OBJECT (parser_capsfilter), "caps", parser_caps, NULL);

  /* unref caps after usage */
  gst_caps_unref (parser_caps);

  /* Add all elements into the pipeline */
  gst_bin_add_many (GST_BIN (pipeline), source, demuxer, parser,
      parser_capsfilter, payloader, sink, NULL);

  /* Link the elements together */
  if (gst_element_link (source, demuxer) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }
  if (gst_element_link_many (parser, parser_capsfilter, payloader, sink,
          NULL) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  /* Dynamic link */
  g_signal_connect (demuxer, "pad-added", G_CALLBACK (on_pad_added), parser);

  /* Set the pipeline to "playing" state */
  g_print ("Now streaming %s...:\n", input_file);
  if (gst_element_set_state (pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (pipeline);
    return -1;
  }

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
  g_print ("Returned, stopping streaming...\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_print ("Deleting pipeline...\n");
  gst_object_unref (GST_OBJECT (pipeline));
  return 0;
}
