/* Copyright (c) 2023 Renesas Electronics Corporation and/or its affiliates */
/* SPDX-License-Identifier: MIT-0 */

#include <gst/gst.h>
#include <stdio.h>
#include <stdbool.h>

#define BITRATE             128000     /* Bitrate averaging */
#define SAMPLE_RATE         44100      /* Sample rate  of audio file*/
#define CHANNEL             2          /* Channel*/
#define INPUT_FILE          "/home/media/audios/Rondo_Alla_Turka_F32LE_44100" \
                            "_stereo.raw"
#define OUTPUT_FILE         "/home/media/audios/ENCODE_Rondo_Alla_Turka.ogg"
#define FORMAT              "F32LE"

typedef struct tag_user_data
{
  GstElement *pipeline;
  GstElement *source;
  GstElement *capsfilter;
  GstElement *encoder;
  GstElement *parser;
  GstElement *muxer;
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
  /* Set the bitrate to 128kbps to the encode element */
  g_object_set (G_OBJECT (data->encoder), "bitrate", BITRATE, NULL);

  /* Set the input filename to the source element */
  g_object_set (G_OBJECT (data->source), "location", INPUT_FILE, NULL);

  /* Set the output filename to the source element */
  g_object_set (G_OBJECT (data->sink), "location", OUTPUT_FILE, NULL);

  /* Create a simple caps */
  caps =
      gst_caps_new_simple ("audio/x-raw",
      "format", G_TYPE_STRING, FORMAT,
      "rate", G_TYPE_INT, SAMPLE_RATE, "channels", G_TYPE_INT, CHANNEL, NULL);
  /* Set the caps option to the caps-filter element */
  g_object_set (G_OBJECT (data->capsfilter), "caps", caps, NULL);
  gst_caps_unref (caps);
}

int
setup_pipeline (UserData *data)
{
  /* set element properties */
  set_element_properties (data);

  /* Add the elements into the pipeline and link them together */
  /* file-source -> caps-filter -> vorbis-encoder -> vorbis-parser
   * -> OGG-muxer -> file-output */
  gst_bin_add_many (GST_BIN (data->pipeline), data->source, data->capsfilter,
      data->encoder, data->parser, data->muxer, data->sink, NULL);

  if (gst_element_link_many (data->source, data->capsfilter, data->encoder,
          data->parser, data->muxer, data->sink, NULL) != TRUE) {
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

  if (!is_file_exist(INPUT_FILE))
  {
    g_printerr("Cannot find input file: %s. Exiting.\n", INPUT_FILE);
    return -1;
  }

  /* Initialization */
  gst_init (&argc, &argv);

  /* Create GStreamer elements */
  user_data.pipeline = gst_pipeline_new ("audio-encode");
  user_data.source = gst_element_factory_make ("filesrc", "file-source");
  user_data.capsfilter = gst_element_factory_make ("capsfilter",
                             "caps-filter");
  user_data.encoder = gst_element_factory_make ("vorbisenc", "vorbis-encoder");
  user_data.parser = gst_element_factory_make ("vorbisparse", "vorbis-parser");
  user_data.muxer = gst_element_factory_make ("oggmux", "OGG-muxer");
  user_data.sink = gst_element_factory_make ("filesink", "file-output");

  if (!user_data.pipeline || !user_data.source || !user_data.encoder ||
          !user_data.capsfilter || !user_data.parser || !user_data.muxer ||
          !user_data.sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  if (setup_pipeline (&user_data) != TRUE) {
    gst_object_unref (user_data.pipeline);
    return -1;
  }

  /* Set the pipeline to "playing" state */
  g_print ("Now start encode file: %s\n", INPUT_FILE);
  if (gst_element_set_state (user_data.pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (user_data.pipeline);
    return -1;
  }

  /* Waiting */
  g_print ("Encoding...");
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

  g_print ("Returned, please wait for the writing file process....\n");
  gst_element_set_state (user_data.pipeline, GST_STATE_NULL);
  g_print ("Deleting pipeline...\n");
  gst_object_unref (GST_OBJECT (user_data.pipeline));
  g_print ("Completed. The output file available at: %s\n", OUTPUT_FILE);

  return 0;
}
