#include <gst/gst.h>

#define BITRATE          10485760
#define CONTROL_RATE     2   /* Constant bitrate */
#define BLOCKSIZE        576000 /* Blocksize of NV12 is width*heigh*3/2 */
#define INPUT_FILE       "/home/media/videos/h264-wvga-30.yuv"
#define OUTPUT_FILE      "/home/media/videos/ENCODE_h264-wvga-30.h264"
#define VIDEO_FORMAT     "NV12"
#define LVDS_WIDTH       800
#define LVDS_HEIGHT      480
#define FRAMERATE        30
#define TYPE_FRACTION    1

int
main (int argc, char *argv[])
{
  GstElement *pipeline, *source, *capsfilter, *encoder, *sink;
  GstCaps *caps;
  GstBus *bus;
  GstMessage *msg;

  const gchar *input_file = INPUT_FILE;
  const gchar *output_file = OUTPUT_FILE;

  /* Initialization */
  gst_init (&argc, &argv);

  /* Create GStreamer elements */
  pipeline = gst_pipeline_new ("video-encode");
  source = gst_element_factory_make ("filesrc", "file-source");
  capsfilter = gst_element_factory_make ("capsfilter", "caps-filter");
  encoder = gst_element_factory_make ("omxh264enc", "H264-encoder");
  sink = gst_element_factory_make ("filesink", "file-output");

  if (!pipeline || !source || !capsfilter || !encoder || !sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  /* Set up the pipeline */

  /* Set the input filename to the source element, blocksize of NV12 is width*heigh*3/2 */
  g_object_set (G_OBJECT (source), "location", input_file, NULL);
  g_object_set (G_OBJECT (source), "blocksize", BLOCKSIZE, NULL);

  /* Set the caps option to the caps-filter element */
  caps =
      gst_caps_new_simple ("video/x-raw",
      "format", G_TYPE_STRING, VIDEO_FORMAT,
      "framerate", GST_TYPE_FRACTION, FRAMERATE, TYPE_FRACTION,
      "width", G_TYPE_INT, LVDS_WIDTH, "height", G_TYPE_INT, LVDS_HEIGHT, NULL); /* The raw video file is in NV12 format, resolution 800x480*/
  g_object_set (G_OBJECT (capsfilter), "caps", caps, NULL);
  gst_caps_unref (caps);

  /* Set the H.264 encoder options: constant control-rate, 10Mbps target bitrate and zero num-p-frames to the encoder element*/
  g_object_set (G_OBJECT (encoder), "control-rate", CONTROL_RATE, NULL);
  g_object_set (G_OBJECT (encoder), "target-bitrate", BITRATE, NULL);
  g_object_set (G_OBJECT (encoder), "num-p-frames", 0, NULL);

  /* Set the output filename to the sink element */
  g_object_set (G_OBJECT (sink), "location", output_file, NULL);

  /* Add all elements into the pipeline */
  /* file-source | caps-filter | H264-encoder | file-output */
  gst_bin_add_many (GST_BIN (pipeline), source, capsfilter, encoder, sink,
      NULL);

  /* Link the elements together */
  /* file-source -> caps-filter -> H264-encoder -> file-output */
  if (gst_element_link_many (source, capsfilter, encoder, sink, NULL) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  /* Set the pipeline to "playing" state */
  g_print ("Now encoding file %s:\n", input_file);
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
  gst_object_unref (bus);

  /* Note that because input timeout is GST_CLOCK_TIME_NONE, 
     the gst_bus_timed_pop_filtered() function will block forever untill a 
     matching message was posted on the bus (GST_MESSAGE_ERROR or 
     GST_MESSAGE_EOS). */

  /* Encode end. Clean up nicely */
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

  g_print ("Returned, stopping conversion...\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);
  g_print ("Deleting pipeline...\n");
  gst_object_unref (GST_OBJECT (pipeline));
  g_print ("Succeeded. Encoded file available at: %s\n", output_file);
  return 0;
}
