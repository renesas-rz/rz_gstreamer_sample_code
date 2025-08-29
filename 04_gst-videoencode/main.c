/* Copyright (c) 2023 Renesas Electronics Corporation and/or its affiliates */
/* SPDX-License-Identifier: MIT-0 */

#include <gst/gst.h>
#include <stdbool.h>
#include <stdio.h>

#define BITRATE          10485760
#define CONTROL_RATE     2   /* Constant bitrate */
#define BLOCKSIZE        1382400 /* Blocksize of NV12 is width*heigh*3/2 */
#define INPUT_FILE       "/home/media/videos/sintel_trailer-720p.yuv"
#define OUTPUT_FILE      "/home/media/videos/ENCODE_h264-720p.264"
#define VIDEO_FORMAT     "NV12"
#define LVDS_WIDTH       1280
#define LVDS_HEIGHT      720
#define FRAMERATE        30
#define TYPE_FRACTION    1

typedef struct tag_user_data
{
  GstElement *pipeline;
  GstElement *source;
  GstElement *capsfilter;
  GstElement *encoder;
  GstElement *sink;
} UserData;

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

void
set_element_properties (UserData *data)
{
  GstCaps *caps;

  /* Set the input filename to the source element,
   * blocksize of NV12 is width*heigh*3/2 */
  g_object_set (G_OBJECT (data->source), "location", INPUT_FILE, NULL);
  g_object_set (G_OBJECT (data->source), "blocksize", BLOCKSIZE, NULL);

  /* Set the caps option to the caps-filter element */
  caps =
      gst_caps_new_simple ("video/x-raw",
      "format", G_TYPE_STRING, VIDEO_FORMAT,
      "framerate", GST_TYPE_FRACTION, FRAMERATE, TYPE_FRACTION,
      "width", G_TYPE_INT, LVDS_WIDTH, "height", G_TYPE_INT, LVDS_HEIGHT,
      NULL); /* The raw video file is in NV12 format, resolution 1280x720*/
  g_object_set (G_OBJECT (data->capsfilter), "caps", caps, NULL);
  gst_caps_unref (caps);

  /* Set the H.264 encoder options: constant control-rate,
   * 10Mbps target bitrate to the encoder element*/
  g_object_set (G_OBJECT (data->encoder), "control-rate", CONTROL_RATE, NULL);
  g_object_set (G_OBJECT (data->encoder), "target-bitrate", BITRATE, NULL);

  /* Set the output filename to the sink element */
  g_object_set (G_OBJECT (data->sink), "location", OUTPUT_FILE, NULL);
}

int
setup_pipeline (UserData *data)
{
  /* set element properties */
  set_element_properties (data);

  /* Add the elements into the pipeline and link them together */
  /* file-source -> caps-filter -> H264-encoder -> file-output */
  gst_bin_add_many (GST_BIN (data->pipeline), data->source, data->capsfilter,
      data->encoder, data->sink, NULL);

  if (gst_element_link_many (data->source, data->capsfilter,
          data->encoder, data->sink, NULL) != TRUE) {
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
  GstBus *bus;
  GstMessage *msg;
  UserData user_data;

  if (!is_file_exist(INPUT_FILE))
  {
    g_printerr("Cannot find input file: %s. Exiting.\n", INPUT_FILE);
    return -1;
  }

  /* Initialization */
  gst_init (&argc, &argv);

  /* Create GStreamer elements */
  user_data.pipeline = gst_pipeline_new ("video-encode");
  user_data.source = gst_element_factory_make ("filesrc", "file-source");
  user_data.capsfilter = gst_element_factory_make ("capsfilter",
                             "caps-filter");
  user_data.encoder = gst_element_factory_make ("omxh264enc", "H264-encoder");
  user_data.sink = gst_element_factory_make ("filesink", "file-output");

  if (!user_data.pipeline || !user_data.source || !user_data.capsfilter ||
          !user_data.encoder || !user_data.sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  if (setup_pipeline (&user_data) != TRUE) {
    gst_object_unref (user_data.pipeline);
    return -1;
  }

  /* Set the pipeline to "playing" state */
  g_print ("Now encoding file: %s\n", INPUT_FILE);
  if (gst_element_set_state (user_data.pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (user_data.pipeline);
    return -1;
  }

  /* Waiting */
  g_print ("Running...\n");
  bus = gst_element_get_bus (user_data.pipeline);
  msg =
      gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
      GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
  gst_object_unref (bus);

  /* Note that because input timeout is GST_CLOCK_TIME_NONE,
     the gst_bus_timed_pop_filtered() function will block forever untill a
     matching message was posted on the bus (GST_MESSAGE_ERROR or
     GST_MESSAGE_EOS). */

  if (msg != NULL) {
    parse_message (msg);
    gst_message_unref (msg);
  }

  g_print ("Returned, stopping conversion...\n");
  gst_element_set_state (user_data.pipeline, GST_STATE_NULL);
  g_print ("Deleting pipeline...\n");
  gst_object_unref (GST_OBJECT (user_data.pipeline));
  g_print ("Succeeded. Encoded file available at: %s\n", OUTPUT_FILE);

  return 0;
}
