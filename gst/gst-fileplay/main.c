#include <stdio.h>
#include <string.h>
#include <gst/gst.h>
#include <stdlib.h>

#define INPUT_FILE                 "/home/media/videos/sintel_trailer-720p.mp4"
#define LOCATION_FILE_WESTON_INFO  "/home/media/data_westoninfo.txt"
#define COMMAND_GET_WESTON_INFO    "weston-info >& /home/media/data_westoninfo.txt"
#define SUBSTRING_WIDTH            "width: "
#define SUBSTRING_HEIGHT           "height: "
#define SUBSTRING_REFRESH          "refresh"
#define SUBSTRING_FLAGS            "flags: current"
#define POSITION_X           0
#define POSITION_Y           0
#define LENGTH_STRING        10
#define INDEX_WESTON_WIDTH   6
#define INDEX_WESTON_HEIGHT  23
#define INDEX                0

/* Count number of available screens */
int count_screens = 0;
/* Declare variable arrays for resolution of screen */
int availbe_width_screen[10];
int availbe_height_screen[10];

/* Structure to contain decoder queue information, so we can pass it to callbacks */
typedef struct _CustomData
{
  GstElement *video_queue;
  GstElement *audio_queue;
} CustomData;

/*
 * 
 * name: get_resolution_from_weston_info
 * Get resolution from "weston-info" 
 * 
 */
static void get_resolution_from_weston_info(int *availbe_width_screen, int *availbe_height_screen)
{
  /* Initial variables  */
  char *line = NULL;
  size_t len, read;
  FILE *fp;
  char p_save_res_line[100];

  /* 
   * To show the usage of system() function to list down all weston-info
   * in the specified file.
   */
  system(COMMAND_GET_WESTON_INFO);

  /* Open data_westoninfo.txt to read width and height */
  fp = fopen(LOCATION_FILE_WESTON_INFO,"rt");

  if(fp == NULL) {
    g_printerr ("Can't open file.\n");
    exit(1);
  }else {
    while((read = getline(&line, &len, fp)) != -1) {
      /*Find the first occurrence of the substring needle in the string. */
      char *p_width   = strstr(line, SUBSTRING_WIDTH);
      char *p_height  = strstr(line, SUBSTRING_HEIGHT);
      char *p_refresh = strstr(line, SUBSTRING_REFRESH);

      /* 
       * Pointer "p_width", "p_height" and "p_refresh" to the first occurrence in "line" of the 
       * entire sequence of characters specified in "width: ", "height: " and "refresh"
       */
      if( p_width && p_height && p_refresh ) {
        /* 
         * Pointer "p_save_res_line" to "p_width" destination array where the content is to be copied.
         * Because calling "getline" function that makes a change for pointer "p_width".
         */
        strncpy(p_save_res_line, p_width, strlen(p_width));

        /* Read more line to check resolution of the screen if it is available.*/
        if((read = getline(&line, &len, fp)) == -1) {
          break;
        }

        /*Find the first occurrence of the substring needle in the string. */
        char *p_flags_current = strstr(line, SUBSTRING_FLAGS);

        /* 
         * Pointer "p_flags_current" to the first occurrence in "line" of the 
         * entire sequence of characters specified in "flags: current".
         */
        if( p_flags_current ) {
          char str_width[LENGTH_STRING], str_height[LENGTH_STRING];
          char *ptr;
  
          /* Get available width of screen from the string. */
          strncpy(str_width, p_save_res_line + INDEX_WESTON_WIDTH, LENGTH_STRING);
          /* Convert sub-string to long integer. */
          availbe_width_screen[count_screens] = strtol(str_width, &ptr, LENGTH_STRING);

          /* Get available height of screen from the string. */
          strncpy(str_height, p_save_res_line + INDEX_WESTON_HEIGHT, LENGTH_STRING);
          /* Convert sub-string to long integer. */
          availbe_height_screen[count_screens] = strtol(str_height, &ptr, LENGTH_STRING);

          count_screens++ ;
        }
      }
    }
  }

  /* Close the file */
  fclose(fp);
}

static void
on_pad_added (GstElement * element, GstPad * pad, gpointer data)
{
  CustomData *decoders = (CustomData *) data;
  GstPad *sinkpad;
  GstCaps *new_pad_caps = NULL;
  GstStructure *new_pad_struct = NULL;
  const gchar *new_pad_type = NULL;

  new_pad_caps = gst_pad_query_caps (pad, NULL);
  new_pad_struct = gst_caps_get_structure (new_pad_caps, INDEX);
  new_pad_type = gst_structure_get_name (new_pad_struct);
  /* NOTE:
     gst_pad_query_caps: increase the ref count, need to be unref late.
     gst_caps_get_structure: no need to free or unref, it belongs to the GstCaps.
     gst_structure_get_name: no need to free or unref.
   */

  g_print ("Received new pad '%s' from '%s': %s\n", GST_PAD_NAME (pad),
      GST_ELEMENT_NAME (element), new_pad_type);

  if (g_str_has_prefix (new_pad_type, "audio")) {
    /* In case link this pad with the AAC-decoder sink pad */
    sinkpad = gst_element_get_static_pad (decoders->audio_queue, "sink");
    gst_pad_link (pad, sinkpad);
    gst_object_unref (sinkpad);
    g_print ("Audio pad linked!\n");
  } else if (g_str_has_prefix (new_pad_type, "video")) {
    /* In case link this pad with the omxh264-decoder sink pad */
    sinkpad = gst_element_get_static_pad (decoders->video_queue, "sink");
    gst_pad_link (pad, sinkpad);
    gst_object_unref (sinkpad);
    g_print ("Video pad linked!\n");
  } else {
    g_printerr ("Unexpected pad received.\n");
  }

  if (new_pad_caps != NULL) {
    gst_caps_unref (new_pad_caps);
  }
}

int
main (int argc, char *argv[])
{
  GstElement *pipeline, *source, *demuxer;
  GstElement *video_queue, *video_parser, *video_decoder, *video_sink;
  GstElement *audio_queue, *audio_decoder, *audio_sink;
  CustomData decoders_data;

  GstBus *bus;
  GstMessage *msg;

  const gchar *input_file = INPUT_FILE;

  /* Get out-width and out-height for the out video */
  get_resolution_from_weston_info(availbe_width_screen, availbe_height_screen);

  /* Initialisation */
  gst_init (&argc, &argv);

  /* Create gstreamer elements */
  pipeline = gst_pipeline_new ("file-play");
  source = gst_element_factory_make ("filesrc", "file-source");
  demuxer = gst_element_factory_make ("qtdemux", "qt-demuxer");
  /* elements for Video thread */
  video_queue = gst_element_factory_make ("queue", "video-queue");
  video_parser = gst_element_factory_make("h264parse", "h264-parser");
  video_decoder = gst_element_factory_make ("omxh264dec", "omxh264-decoder");
  video_sink = gst_element_factory_make ("waylandsink", "video-output");
  /* elements for Audio thread */
  audio_queue = gst_element_factory_make ("queue", "audio-queue");
  audio_decoder = gst_element_factory_make ("faad", "aac-decoder");
  audio_sink = gst_element_factory_make ("alsasink", "audio-output");

  if (!pipeline || !source || !demuxer
      || !video_queue || !video_decoder || !video_sink
      || !audio_queue || !audio_decoder || !audio_sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  /* Set up the pipeline */

  /* Set the input filename to the source element */
  g_object_set (G_OBJECT (source), "location", input_file, NULL);

  /* Set position for displaying (0, 0) */
  g_object_set (G_OBJECT (video_sink), "position-x", POSITION_X, "position-y", POSITION_Y, NULL);

  /* Set out-width and out-height for the out video */
  g_object_set (G_OBJECT (video_sink), "out-width", availbe_width_screen[0], "out-height", availbe_height_screen[0], NULL);

  /* Add all elements into the pipeline:
     file-source | qt-demuxer
     <1> | video-queue | omxh264-decoder | video-output
     <2> | audio-queue | aac-decoder | audio-output
   */
  gst_bin_add_many (GST_BIN (pipeline), source, demuxer,
      video_queue, video_parser, video_decoder, video_sink,
      audio_queue, audio_decoder, audio_sink, NULL);

  /* Link the elements together:
     - file-source -> qt-demuxer 
     - video-queue -> omxh264-decoder -> video-output    
     - audio-queue -> aac-decoder -> audio-output
   */
  if (gst_element_link (source, demuxer) != TRUE) {
    g_printerr ("Source and demuxer could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }
  if (gst_element_link_many (video_queue, video_parser, video_decoder, video_sink,
          NULL) != TRUE) {
    g_printerr ("Video elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }
  if (gst_element_link_many (audio_queue, audio_decoder, audio_sink,
          NULL) != TRUE) {
    g_printerr ("Audio elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  decoders_data.audio_queue = audio_queue;
  decoders_data.video_queue = video_queue;
  g_signal_connect (demuxer, "pad-added", G_CALLBACK (on_pad_added),
      &decoders_data);

  /* note that the demuxer will be linked to the decoder dynamically.
     The reason is that Qt may contain various streams (for example
     video, audio and subtitle). The source pad(s) will be created at run time,
     by the demuxer when it detects the amount and nature of streams.
     Therefore we connect a callback function which will be executed
     when the "pad-added" is emitted. */

  /* set the piline to "playing" state */
  g_print ("Now playing %s:\n", input_file);
  if (gst_element_set_state (pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (pipeline);
    return -1;
  }
  /* Iterate */
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

  /* Clean up nicely */
  g_print ("Returned, stopping playback\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_print ("Deleting pipeline\n");
  gst_object_unref (GST_OBJECT (pipeline));

  return 0;
}
