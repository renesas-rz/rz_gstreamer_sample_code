#include <gst/gst.h>

#define BITRATE_OMXH264ENC 10485760 /* Target bitrate of the encoder element - omxh264enc */
#define WIDTH_SIZE         640             /* The output data of v4l2src in this application will be a raw video with 640x480 size */
#define HEIGHT_SIZE        480 
#define INPUT_FILE         "/home/media/videos/sintel_trailer-720p.mp4"
#define OUTPUT_FILE        "/home/media/videos/CONVERT_video.mp4"

static void
on_pad_added (GstElement * element, GstPad * pad, gpointer data)
{
  GstPad *sinkpad;
  GstElement *parser = (GstElement *) data;

  /* We can now link this pad with the H.264 parser sink pad */
  g_print ("Dynamic pad created, linking demuxer/parser\n");

  sinkpad = gst_element_get_static_pad (parser, "sink");
  /* Link the input pad and the new request pad */
  gst_pad_link (pad, sinkpad);

  gst_object_unref (sinkpad);
}

/* Function is used to link request pad and a static pad */
static void
link_to_multiplexer (GstPad * tolink_pad, GstElement * mux)
{
  GstPad *pad;
  gchar *srcname, *sinkname;

  srcname = gst_pad_get_name (tolink_pad);
  pad = gst_element_get_compatible_pad (mux, tolink_pad, NULL);
  /* Link the input pad and the new request pad */
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
  GstElement *pipeline, *source, *demuxer, *parser1, *decoder,
      *converter, *conv_capsfilter, *encoder, *parser2, *muxer, *sink;
  GstBus *bus;
  GstMessage *msg;
  GstPad *srcpad;
  GstCaps *conv_caps;

  const gchar *input_file = INPUT_FILE;
  const gchar *output_file = OUTPUT_FILE;

  /* Initialization */
  gst_init (&argc, &argv);

  /* Create GStreamer elements */
  pipeline = gst_pipeline_new ("video-convert");
  source = gst_element_factory_make ("filesrc", "video-src");
  demuxer = gst_element_factory_make ("qtdemux", "mp4-demuxer");
  parser1 = gst_element_factory_make ("h264parse", "h264-parser-1");
  decoder = gst_element_factory_make ("omxh264dec", "video-decoder");
  converter = gst_element_factory_make ("vspfilter", "video-converter");
  conv_capsfilter = gst_element_factory_make ("capsfilter", "convert_caps");
  encoder = gst_element_factory_make ("omxh264enc", "video-encoder");
  parser2 = gst_element_factory_make ("h264parse", "h264-parser-2");
  muxer = gst_element_factory_make ("qtmux", "mp4-muxer");
  sink = gst_element_factory_make ("filesink", "file-output");

  if (!pipeline || !source || !demuxer || !parser1 || !decoder || !converter
      || !conv_capsfilter || !encoder || !parser2 || !muxer || !sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  /* Set input video device file of the source element - v4l2src */
  g_object_set (G_OBJECT (source), "location", input_file, NULL);

  /* Set target-bitrate property of the encoder element - omxh264enc */
  g_object_set (G_OBJECT (encoder), "target-bitrate", BITRATE_OMXH264ENC, NULL);

  /* Set output file location of the sink element - filesink */
  g_object_set (G_OBJECT (sink), "location", output_file, NULL);

  /* create simple caps */
  conv_caps =
      gst_caps_new_simple ("video/x-raw", "width", G_TYPE_INT, WIDTH_SIZE, "height",
      G_TYPE_INT, HEIGHT_SIZE, NULL);

  /* set caps property for capsfilters */
  g_object_set (G_OBJECT (conv_capsfilter), "caps", conv_caps, NULL);

  /* unref caps after usage */
  gst_caps_unref (conv_caps);

  /* Add all elements into the pipeline */
  gst_bin_add_many (GST_BIN (pipeline), source, demuxer, parser1, decoder,
      converter, conv_capsfilter, encoder, parser2, muxer, sink, NULL);

  /* Link the elements together */
  if (gst_element_link (source, demuxer) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }
  if (gst_element_link_many (parser1, decoder, converter, conv_capsfilter, encoder,
          parser2, NULL) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  if (gst_element_link (muxer, sink) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }
  /* Dynamic link */
  g_signal_connect (demuxer, "pad-added", G_CALLBACK (on_pad_added), parser1);

  /* link srcpad of h264parse to request pad of qtmuxer */
  srcpad = gst_element_get_static_pad (parser2, "src");
  link_to_multiplexer (srcpad, muxer);
  gst_object_unref (srcpad);

  /* Set the pipeline to "playing" state */
  g_print ("Now converting...\n");
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
  g_print ("Returned, stopping conversion...\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_print ("Deleting pipeline...\n");
  gst_object_unref (GST_OBJECT (pipeline));
  g_print ("Succeeded. Output file available at: %s\n", output_file);
  return 0;
}
