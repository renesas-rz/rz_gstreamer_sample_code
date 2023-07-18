#include <gst/gst.h>
#include <stdio.h>
#include <stdbool.h>

#define BITRATE             128000     /* Bitrate averaging */
#define SAMPLE_RATE         44100      /* Sample rate  of audio file*/
#define CHANNEL             2          /* Channel*/
#define INPUT_FILE          "/home/media/audios/Rondo_Alla_Turka_F32LE_44100_stereo.raw"
#define OUTPUT_FILE         "/home/media/audios/ENCODE_Rondo_Alla_Turka.ogg"
#define FORMAT              "F32LE"

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

int
main (int argc, char *argv[])
{
  GstElement *pipeline, *source, *capsfilter, *encoder, *parser, *muxer, *sink;
  GstCaps *caps;
  GstBus *bus;
  GstMessage *msg;

  const gchar *input_file = INPUT_FILE;
  const gchar *output_file = OUTPUT_FILE;

  if (!is_file_exist(input_file))
  {
    g_printerr("Cannot find input file: %s. Exiting.\n", input_file);
    return -1;
  }

  /* Initialization */
  gst_init (&argc, &argv);

  /* Create GStreamer elements */
  pipeline = gst_pipeline_new ("audio-encode");
  source = gst_element_factory_make ("filesrc", "file-source");
  capsfilter = gst_element_factory_make ("capsfilter", "caps-filter");
  encoder = gst_element_factory_make ("vorbisenc", "vorbis-encoder");
  parser = gst_element_factory_make ("vorbisparse", "vorbis-parser");
  muxer = gst_element_factory_make ("oggmux", "OGG-muxer");
  sink = gst_element_factory_make ("filesink", "file-output");

  if (!pipeline || !source || !encoder || !capsfilter || !parser || !muxer
      || !sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  /* Set up the pipeline */

  /* Set the input filename to the source element */
  g_object_set (G_OBJECT (source), "location", input_file, NULL);

  /* Set the bitrate to 128kbps to the encode element */
  g_object_set (G_OBJECT (encoder), "bitrate", BITRATE, NULL);

  /* Set the output filename to the source element */
  g_object_set (G_OBJECT (sink), "location", output_file, NULL);

  /* Create a simple caps */
  caps =
      gst_caps_new_simple ("audio/x-raw",
      "format", G_TYPE_STRING, FORMAT,
      "rate", G_TYPE_INT, SAMPLE_RATE, "channels", G_TYPE_INT, CHANNEL, NULL);

  /* Set the caps option to the caps-filter element */
  g_object_set (G_OBJECT (capsfilter), "caps", caps, NULL);
  gst_caps_unref (caps);

  /* Add all elements into the pipeline */
  /* file-source | vorbis-encoder | vorbis-parser | OGG-muxer | file-output */
  gst_bin_add_many (GST_BIN (pipeline), source, capsfilter, encoder,
      parser, muxer, sink, NULL);

  /* Link the elements together */
  if (gst_element_link_many (source, capsfilter, encoder, parser, muxer,
          sink, NULL) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  /* Set the pipeline to "playing" state */
  g_print ("Now start encode file: %s\n", input_file);
  if (gst_element_set_state (pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  /* Waiting */
  g_print ("Encoding...");
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
  g_print ("Returned, please wait for the writing file process....\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_print ("Deleting pipeline...\n");
  gst_object_unref (GST_OBJECT (pipeline));
  g_print ("Completed. The output file available at: %s\n", output_file);
  return 0;
}
