#include <gst/gst.h>

#define BITRATE_OMXH264ENC 10485760 /* Target bitrate of the encoder element - omxh264enc */
#define WIDTH_SIZE  640             /* The output data of v4l2src in this application will be a raw video with 640x480 size */
#define HEIGHT_SIZE 480 

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
  GstElement *source, *camera_capsfilter, *converter, *convert_capsfilter,
      *encoder, *parser, *muxer, *sink;
  GstBus *bus;
  GstMessage *msg;
  GstPad *srcpad;
  GstCaps *camera_caps, *convert_caps;

  const gchar *output_file = "/home/media/videos/RECORD_USB-camera.mp4";

  if (argv[1] == NULL) {
    g_print ("No input! Please input video device for this app\n");
    return -1;
  }

  /* Initialization */
  gst_init (&argc, &argv);

  /* Create GStreamer elements */
  pipeline = gst_pipeline_new ("video-record");
  source = gst_element_factory_make ("v4l2src", "camera-source");
  camera_capsfilter = gst_element_factory_make ("capsfilter", "camera_caps");
  converter = gst_element_factory_make ("videoconvert", "video-converter");
  convert_capsfilter = gst_element_factory_make ("capsfilter", "convert_caps");
  encoder = gst_element_factory_make ("omxh264enc", "video-encoder");
  parser = gst_element_factory_make ("h264parse", "h264-parser");
  muxer = gst_element_factory_make ("qtmux", "mp4-muxer");
  sink = gst_element_factory_make ("filesink", "file-output");

  if (!pipeline || !source || !camera_capsfilter || !converter
      || !convert_capsfilter || !encoder || !parser || !muxer || !sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  /* Set input video device file of the source element - v4l2src */
  g_object_set (G_OBJECT (source), "device", argv[1], NULL);

  /* Set target-bitrate property of the encoder element - omxh264enc */
  g_object_set (G_OBJECT (encoder), "target-bitrate", BITRATE_OMXH264ENC, NULL);

  /* Set output file location of the sink element - filesink */
  g_object_set (G_OBJECT (sink), "location", output_file, NULL);

  /* create simple caps */
  camera_caps =
      gst_caps_new_simple ("video/x-raw", "width", G_TYPE_INT, WIDTH_SIZE, "height",
      G_TYPE_INT, HEIGHT_SIZE, NULL);
  convert_caps =
      gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING, "NV12",
      NULL);

  /* set caps property for capsfilters */
  g_object_set (G_OBJECT (camera_capsfilter), "caps", camera_caps, NULL);
  g_object_set (G_OBJECT (convert_capsfilter), "caps", convert_caps, NULL);

  /* unref caps after usage */
  gst_caps_unref (camera_caps);
  gst_caps_unref (convert_caps);

  /* Add all elements into the pipeline */
  gst_bin_add_many (GST_BIN (pipeline), source, camera_capsfilter, converter,
      convert_capsfilter, encoder, parser, muxer, sink, NULL);

  /* Link the elements together */
  if (gst_element_link_many (source, camera_capsfilter, converter,
          convert_capsfilter, encoder, parser, NULL) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }
  if (gst_element_link (muxer, sink) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  /* link srcpad of h264parse to request pad of qtmuxer */
  srcpad = gst_element_get_static_pad (parser, "src");
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
