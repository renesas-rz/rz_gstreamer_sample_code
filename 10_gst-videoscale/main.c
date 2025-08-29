/* Copyright (c) 2023 Renesas Electronics Corporation and/or its affiliates */
/* SPDX-License-Identifier: MIT-0 */

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
  GstElement *pipeline;
  GstElement *source;
  GstElement *demuxer;
  GstElement *parser1;
  GstElement *decoder;
  GstElement *filter;
  GstElement *capsfilter;
  GstElement *encoder;
  GstElement *parser2;
  GstElement *muxer;
  GstElement *sink;

  const gchar *input_file;
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
static int
link_to_muxer (GstElement *up_element, GstElement *muxer)
{
  gchar *src_name, *sink_name;
  GstPad *src_pad, *req_pad;

  src_pad = gst_element_get_static_pad (up_element, "src");

  src_name = gst_pad_get_name (src_pad);
  req_pad = gst_element_get_compatible_pad (muxer, src_pad, NULL);
  if (!req_pad) {
    g_print ("Request pad can not be found.\n");
    return FALSE;
  }
  sink_name = gst_pad_get_name (req_pad);
  /* Link the source pad and the new request pad */
  gst_pad_link (src_pad, req_pad);
  g_print ("Request pad %s was created and linked to %s\n", sink_name, src_name);

  gst_object_unref (GST_OBJECT (src_pad));
  gst_object_unref (GST_OBJECT (req_pad));
  g_free (src_name);
  g_free (sink_name);

  return TRUE;
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

void
set_element_properties (UserData *data)
{
  /* Set input video file of the source element - filesrc */
  g_object_set (G_OBJECT (data->source), "location", data->input_file, NULL);

  /* Set dmabuf-use mode of the filter element - vspmfilter */
  g_object_set (G_OBJECT (data->filter), "dmabuf-use", TRUE, NULL);

  /* Set target-bitrate property of the encoder element - omxh264enc */
  g_object_set (G_OBJECT (data->encoder), "target-bitrate", BITRATE_OMXH264ENC,
      "control-rate", 1, NULL);

  /* Set output file location of the sink element - filesink */
  g_object_set (G_OBJECT (data->sink), "location", OUTPUT_FILE, NULL);
}

int
setup_pipeline (UserData *data)
{
  /* set element properties */
  set_element_properties (data);

  /* Add the elements into the pipeline and link them together */
  gst_bin_add_many (GST_BIN (data->pipeline),
      data->source, data->demuxer, data->parser1, data->decoder,data->filter,
      data->capsfilter, data->encoder, data->parser2, data->muxer, data->sink,
      NULL);
  /* file-src -> mp4-demuxer */
  if (gst_element_link (data->source, data->demuxer) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    return FALSE;
  }
  /* h264-parser-1 -> video-decoder -> video-filter -> video-encoder
   * -> h264-parser-2 */
  if (gst_element_link_many (data->parser1, data->decoder, data->filter,
          data->capsfilter, data->encoder, data->parser2, NULL) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    return FALSE;
  }
  /* mp4-muxer -> file-output */
  if (gst_element_link (data->muxer, data->sink) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    return FALSE;
  }
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
  UserData user_data;
  GstBus *bus;
  GstMessage *msg;
  const char* ext;
  char* file_name;

  if (argc != ARG_COUNT)
  {
    g_print ("Invalid arugments.\n");
    g_print ("Usage: %s <MP4 file> <width> <height> \n",
        argv[ARG_PROGRAM_NAME]);
    return -1;
  }

  user_data.input_file = argv[ARG_INPUT];

  if (!is_file_exist(user_data.input_file))
  {
    g_printerr("Cannot find input file: %s. Exiting.\n", user_data.input_file);
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
  user_data.pipeline = gst_pipeline_new ("video-scale");
  user_data.source = gst_element_factory_make ("filesrc", "video-src");
  user_data.demuxer = gst_element_factory_make ("qtdemux", "mp4-demuxer");
  user_data.parser1 = gst_element_factory_make ("h264parse", "h264-parser-1");
  user_data.decoder = gst_element_factory_make ("omxh264dec", "video-decoder");
  user_data.filter = gst_element_factory_make ("vspmfilter", "video-filter");
  user_data.capsfilter = gst_element_factory_make ("capsfilter", "capsfilter");
  user_data.encoder = gst_element_factory_make ("omxh264enc", "video-encoder");
  user_data.parser2 = gst_element_factory_make ("h264parse", "h264-parser-2");
  user_data.muxer = gst_element_factory_make ("qtmux", "mp4-muxer");
  user_data.sink = gst_element_factory_make ("filesink", "file-output");

  if (!user_data.pipeline || !user_data.source || !user_data.demuxer ||
          !user_data.parser1 || !user_data.decoder || !user_data.filter ||
          !user_data.capsfilter || !user_data.encoder || !user_data.parser2 ||
          !user_data.muxer || !user_data.sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  if (setup_pipeline (&user_data) != TRUE) {
    gst_object_unref (user_data.pipeline);
    return -1;
  }

  user_data.scaled_width = atoi (argv[ARG_WIDTH]);
  user_data.scaled_height = atoi (argv[ARG_HEIGHT]);

  /* Dynamic link */
  g_signal_connect (user_data.demuxer, "pad-added", G_CALLBACK (on_pad_added),
      &user_data);

  /* link source pad of h264parse to request pad of qtmuxer */
  if (link_to_muxer (user_data.parser2, user_data.muxer) != TRUE) {
    g_printerr ("Failed to link to muxer.\n");
    return -1;
  }

  /* Set the pipeline to "playing" state */
  g_print ("Running...\n");
  if (gst_element_set_state (user_data.pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (user_data.pipeline);
    return -1;
  }

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

  g_print ("Returned, stopping scaling...\n");
  gst_element_set_state (user_data.pipeline, GST_STATE_NULL);
  g_print ("Deleting pipeline...\n");
  gst_object_unref (GST_OBJECT (user_data.pipeline));
  g_print ("Succeeded. Please check output file: %s\n", OUTPUT_FILE);

  return 0;
}
