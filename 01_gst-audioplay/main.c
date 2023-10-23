#include <gst/gst.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <libgen.h>

#define AUDIO_SAMPLE_RATE 44100
#define ARG_PROGRAM_NAME 0
#define ARG_INPUT 1
#define ARG_COUNT 2
typedef struct tag_user_data
{
  GstElement *pipeline;
  GstElement *source;
  GstElement *parser;
  GstElement *decoder;
  GstElement *audioresample;
  GstElement *capsfilter;
  GstElement *sink;
  const gchar *input_file;
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

/* get the extention of filename */
const char*
get_filename_ext (const char *filename)
{
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
set_element_properties(UserData *data)
{
  GstCaps *caps;

  /* Set the caps option to the resample caps-filter element */
  caps = gst_caps_new_simple ("audio/x-raw", "rate", G_TYPE_INT,
             AUDIO_SAMPLE_RATE, NULL);
  g_object_set (G_OBJECT (data->capsfilter), "caps", caps, NULL);
  gst_caps_unref (caps);

  /* Set the input filename to the source element */
  g_object_set (G_OBJECT (data->source), "location", data->input_file, NULL);
}

int
setup_pipeline(UserData *data)
{
  /* set element properties */
  set_element_properties(data);

  /* Add the elements into the pipeline and link them together */
  /* file-src -> mp3-parser -> mp3-decoder -> audioresample -> alsa-output */
  gst_bin_add_many (GST_BIN (data->pipeline), data->source, data->parser,
      data->decoder, data->audioresample, data->capsfilter, data->sink, NULL);

  if (gst_element_link_many (data->source, data->parser, data->decoder,
          data->audioresample, data->capsfilter, data->sink, NULL) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    return FALSE;
  }
  return TRUE;
}

int
main (int argc, char *argv[])
{
  UserData user_data;
  GstBus *bus;
  GstMessage *msg;
  const char *ext;
  char *file_name;

  if (argc != ARG_COUNT)
  {
    g_print ("Invalid arugments.\n");
    g_print ("Format: %s <path to file> \n", argv[ARG_PROGRAM_NAME]);
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

  if (strcasecmp ("mp3", ext) != 0)
  {
    g_print ("Invalid extention. This application is used to play an mp3 " \
        "file.\n");
    return -1;
  }
  /* Initialization */
  gst_init (&argc, &argv);

  /* Create GStreamer elements */
  user_data.pipeline = gst_pipeline_new ("audio-play");
  user_data.source = gst_element_factory_make ("filesrc", "file-source");
  user_data.parser = gst_element_factory_make ("mpegaudioparse",
                         "mp3-parser");
  user_data.decoder = gst_element_factory_make ("mpg123audiodec",
                          "mp3-decoder");
  user_data.audioresample = gst_element_factory_make ("audioresample",
                                "audio-resample");
  user_data.capsfilter = gst_element_factory_make ("capsfilter",
                             "resample_capsfilter");
  user_data.sink = gst_element_factory_make ("alsasink", "audio-output");

  if (!user_data.pipeline || !user_data.source ||
          !user_data.parser || !user_data.decoder ||
          !user_data.audioresample || !user_data.capsfilter ||
          !user_data.sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  if (setup_pipeline(&user_data) != TRUE) {
    gst_object_unref (user_data.pipeline);
    return -1;
  }

  /* Set the pipeline to "playing" state */
  g_print ("Now playing file %s:\n", user_data.input_file);
  if (gst_element_set_state (user_data.pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (user_data.pipeline);
    return -1;
  }

  /* Waiting */
  g_print ("Running...\n");
  bus = gst_element_get_bus (user_data.pipeline);
  msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
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
  gst_element_set_state (user_data.pipeline, GST_STATE_NULL);
  g_print ("Deleting pipeline...\n");
  gst_object_unref (GST_OBJECT (user_data.pipeline));
  g_print ("Program end!\n");
  return 0;
}
