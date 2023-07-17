#include <gst/gst.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <libgen.h>
#include <math.h>

#define BITRATE_OMXH264ENC 40000000 /* Target bitrate of the encoder element - omxh264enc */
#define OUTPUT_FILE        "SCALE_video.mp4"
#define ARG_PROGRAM_NAME   0
#define ARG_INPUT          1
#define ARG_WIDTH          2
#define ARG_HEIGHT         3
#define ARG_COUNT          4

typedef struct tag_user_data
{
  GstElement *parser1;
  GstElement *capsfilter;
  int scaled_width;
  int scaled_height;
} UserData;

static void
on_pad_added (GstElement * element, GstPad * pad, gpointer data)
{
  GstPad *sinkpad;
  GstCaps *new_pad_caps = NULL;
  GstStructure *new_pad_struct = NULL;
  const gchar *new_pad_type = NULL;
  UserData *puser_data = (UserData *) data;
  GstCaps *scale_caps;
  int width;
  int height;
  int scaled_width;
  int scaled_height;

  /* Gets the capabilities that pad can produce or consume */
  new_pad_caps = gst_pad_query_caps (pad, NULL);

  /* Gets the structure in caps */
  new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);

  /* Get the name of structure */
  new_pad_type = gst_structure_get_name (new_pad_struct);

  g_print ("Received new pad '%s' from '%s': %s\n", GST_PAD_NAME (pad),
      GST_ELEMENT_NAME (element), new_pad_type);

  if (g_str_has_prefix (new_pad_type, "video/x-h264")) {

    /* Get width of video */
    gst_structure_get_int (new_pad_struct, "width", &width);

    /* Get height of video */
    gst_structure_get_int (new_pad_struct, "height", &height);

    g_print ("Resolution of original video: %dx%d\n", width, height);

    scaled_width = (int) (puser_data->scaled_width);

    scaled_height = (int) (puser_data->scaled_height);

    g_print ("Now scaling video to resolution %dx%d...\n", scaled_width, scaled_height);

    if ((scaled_width > width) || (scaled_height > height)) {
      g_printerr ("Do not support scale up. Exiting... \n");
    }

    /* create simple caps */
    scale_caps =
        gst_caps_new_simple ("video/x-raw", "width", G_TYPE_INT, scaled_width, "height",
        G_TYPE_INT, scaled_height, NULL);

    /* set caps property for capsfilters */
    g_object_set (G_OBJECT (puser_data->capsfilter), "caps", scale_caps, NULL);

    /* unref caps after usage */
    gst_caps_unref (scale_caps);
    /* We can now link this pad with the H.264 parser sink pad */
    g_print ("Dynamic pad created, linking demuxer/parser\n");

    sinkpad = gst_element_get_static_pad (puser_data->parser1, "sink");
    /* Link the input pad and the new request pad */
    gst_pad_link (pad, sinkpad);

    gst_object_unref (sinkpad);

  } else {
    g_printerr ("Unexpected pad received.\n");
  }

  if (new_pad_caps != NULL) {
    gst_caps_unref (new_pad_caps);
  }
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

/*
 *
 * name: is_file_exist
 * Check if the input file exists or not?
 *
 */
bool
is_file_exist(const char *path)
{
  bool result = false;
  FILE *fd = NULL;

  if (path != NULL)
  {
    fd = fopen(path, "r");
    if (fd != NULL)
    {
      result = true;
      fclose(fd);
    }
  }

  return result;
}

/* get the extention of filename */
const char* get_filename_ext (const char *filename) {
  const char* dot = strrchr (filename, '.');
  if ((!dot) || (dot == filename))
  {
    g_print ("Invalid input file.\n");
    return "";
  }
  else
  {
    return dot + 1;
  }
}

int
main (int argc, char *argv[])
{
  GstElement *pipeline, *source, *demuxer, *parser1, *decoder,
      *filter, *capsfilter, *encoder, *parser2, *muxer, *sink;
  GstBus *bus;
  GstMessage *msg;
  GstPad *srcpad;
  const char* ext;
  char* file_name;
  UserData user_data;

  const gchar *output_file = OUTPUT_FILE;

  if (argc != ARG_COUNT)
  {
    g_print ("Invalid arugments.\n");
    g_print ("Usage: %s <MP4 file> <width> <height> \n", argv[ARG_PROGRAM_NAME]);
    return -1;
  }

  const gchar *input_file = argv[ARG_INPUT];

  if (!is_file_exist(input_file))
  {
    g_printerr("Cannot find input file: %s. Exiting.\n", input_file);
    return -1;
  }

  file_name = basename (argv[ARG_INPUT]);
  ext = get_filename_ext (file_name);

  if (strcasecmp ("mp4", ext) != 0)
  {
    g_print ("Unsupported video type. MP4 format is required.\n");
    return -1;
  }

  /* Initialization */
  gst_init (&argc, &argv);

  /* Create GStreamer elements */
  pipeline = gst_pipeline_new ("video-scale");
  source = gst_element_factory_make ("filesrc", "video-src");
  demuxer = gst_element_factory_make ("qtdemux", "mp4-demuxer");
  parser1 = gst_element_factory_make ("h264parse", "h264-parser-1");
  decoder = gst_element_factory_make ("omxh264dec", "video-decoder");
  filter = gst_element_factory_make ("vspmfilter", "video-filter");
  capsfilter = gst_element_factory_make ("capsfilter", "capsfilter");
  encoder = gst_element_factory_make ("omxh264enc", "video-encoder");
  parser2 = gst_element_factory_make ("h264parse", "h264-parser-2");
  muxer = gst_element_factory_make ("qtmux", "mp4-muxer");
  sink = gst_element_factory_make ("filesink", "file-output");

  if (!pipeline || !source || !demuxer || !parser1 || !decoder || !filter
      || !capsfilter || !encoder || !parser2 || !muxer || !sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  /* Set input video file of the source element - filesrc */
  g_object_set (G_OBJECT (source), "location", input_file, NULL);

  /* Set dmabuf-use mode of the filter element - vspmfilter */
  g_object_set (G_OBJECT (filter), "dmabuf-use", TRUE, NULL);

  /* Set target-bitrate property of the encoder element - omxh264enc */
  g_object_set (G_OBJECT (encoder), "target-bitrate", BITRATE_OMXH264ENC,
      "control-rate", 1, NULL);

  /* Set output file location of the sink element - filesink */
  g_object_set (G_OBJECT (sink), "location", output_file, NULL);

  /* Add all elements into the pipeline */
  gst_bin_add_many (GST_BIN (pipeline), source, demuxer, parser1, decoder,
      filter, capsfilter, encoder, parser2, muxer, sink, NULL);

  /* Link the elements together */
  if (gst_element_link (source, demuxer) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }
  if (gst_element_link_many (parser1, decoder, filter, capsfilter, encoder,
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

  user_data.parser1 = parser1;
  user_data.capsfilter = capsfilter;
  user_data.scaled_width = atoi (argv[ARG_WIDTH]);
  user_data.scaled_height = atoi (argv[ARG_HEIGHT]);

  /* Dynamic link */
  g_signal_connect (demuxer, "pad-added", G_CALLBACK (on_pad_added), &user_data);

  /* link srcpad of h264parse to request pad of qtmuxer */
  srcpad = gst_element_get_static_pad (parser2, "src");
  link_to_multiplexer (srcpad, muxer);
  gst_object_unref (srcpad);

  /* Set the pipeline to "playing" state */
  g_print ("Running...\n");
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
  g_print ("Returned, stopping scaling...\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_print ("Deleting pipeline...\n");
  gst_object_unref (GST_OBJECT (pipeline));
  g_print ("Succeeded. Please check output file: %s\n", output_file);
  return 0;
}
