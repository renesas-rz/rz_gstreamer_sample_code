#include <stdio.h>
#include <string.h>
#include <gst/gst.h>
#include <stdlib.h>

#define INPUT_VIDEO_FILE           "/home/media/videos/vga1.h264"
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

/* Count number of available screens */
int count_screens = 0;
/* Declare variable arrays for resolution of screen */
int availbe_width_screen[10];
int availbe_height_screen[10];

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

int
main (int argc, char *argv[])
{
  GstElement *pipeline, *source, *parser, *decoder, *tee;
  GstElement *queue_1, *video_sink_1;
  GstElement *queue_2, *video_sink_2;

  GstPad *req_pad_1, *sink_pad, *req_pad_2;
  GstBus *bus;
  GstMessage *msg;
  GstPadTemplate *tee_src_pad_template;

  const char *input_video_file = INPUT_VIDEO_FILE;

  /* Get out-width and out-height for the out video */
  get_resolution_from_weston_info(availbe_width_screen, availbe_height_screen);

  /* Initialization */
  gst_init (&argc, &argv);

  /* Create GStreamer elements */
  pipeline = gst_pipeline_new ("multiple-display");
  source = gst_element_factory_make ("filesrc", "file-source");
  parser = gst_element_factory_make ("h264parse", "parser");
  decoder = gst_element_factory_make ("omxh264dec", "omxh264-decoder");
  tee = gst_element_factory_make ("tee", "tee-element");

  /* Elements for Video Display 1 */
  queue_1 = gst_element_factory_make ("queue", "queue-1");
  video_sink_1 = gst_element_factory_make ("waylandsink", "video-output-1");

  /* Elements for Video Display 2 */
  queue_2 = gst_element_factory_make ("queue", "queue-2");
  video_sink_2 = gst_element_factory_make ("waylandsink", "video-output-2");

  if (!pipeline || !source || !parser || !decoder || !tee
      || !queue_1 || !video_sink_1
      || !queue_2 || !video_sink_2) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  /* Set up the pipeline */

  /* Set the input file location to the source element */
  g_object_set (G_OBJECT (source), "location", input_video_file, NULL);

  /* The first screen displays from position (0, 0) to (availbe_width_screen[0], availbe_height_screen[0]) */
  /* The second screen displays from position (availbe_width_screen[0], 0) to (availbe_width_screen[1], availbe_height_screen[1]) */

  /* Set display position and size for Display 1  */
  g_object_set (G_OBJECT (video_sink_1), "position-x", POSITION_X, "position-y", POSITION_Y,
       "out-width", availbe_width_screen[0], "out-height", availbe_height_screen[0], NULL);

  /* Set display position and size for Display 2 */
  if( count_screens > 1 ) {
    g_object_set (G_OBJECT (video_sink_2), "position-x", availbe_width_screen[0], "position-y", POSITION_Y,
       "out-width", availbe_width_screen[1], "out-height", availbe_height_screen[1], NULL);
  } else {
    /* With count_screens = 1, two videos overlap on one screen */
    g_object_set (G_OBJECT (video_sink_2), "position-x", POSITION_X, "position-y", POSITION_Y,
       "out-width", availbe_width_screen[0], "out-height", availbe_height_screen[0], NULL);
  }

  /* Add all elements into the pipeline */
  gst_bin_add_many (GST_BIN (pipeline), source, parser, decoder, tee,
      queue_1, video_sink_1,
      queue_2, video_sink_2, NULL);

  /* Link elements together */
  if (gst_element_link_many (source, parser, decoder, tee, NULL) != TRUE) {
    g_printerr ("Source elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }
  if (gst_element_link_many (queue_1, video_sink_1,
          NULL) != TRUE) {
    g_printerr ("Elements of Video Display-1 could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }
  if (gst_element_link_many (queue_2, video_sink_2,
          NULL) != TRUE) {
    g_printerr ("Elements of Video Display-2 could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  /* Get a src pad template of Tee */
  tee_src_pad_template =
      gst_element_class_get_pad_template (GST_ELEMENT_GET_CLASS (tee),
      "src_%u");

  /* Get request pad and manually link for Video Display 1 */
  req_pad_1 = gst_element_request_pad (tee, tee_src_pad_template, NULL, NULL);
  sink_pad = gst_element_get_static_pad (queue_1, "sink");
  if (gst_pad_link (req_pad_1, sink_pad) != GST_PAD_LINK_OK) {
    g_print ("tee link failed!\n");
  }
  gst_object_unref (sink_pad);

  /* Get request pad and manually link for Video Display 2 */
  req_pad_2 = gst_element_request_pad (tee, tee_src_pad_template, NULL, NULL);
  sink_pad = gst_element_get_static_pad (queue_2, "sink");
  if (gst_pad_link (req_pad_2, sink_pad) != GST_PAD_LINK_OK) {
    g_print ("tee link failed!\n");
  }
  gst_object_unref (sink_pad);

  /* Set the pipeline to "playing" state */
  g_print ("Now playing:\n");
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
     the gst_bus_timed_pop_filtered() function will block forever until a 
     matching message was posted on the bus (GST_MESSAGE_ERROR or 
     GST_MESSAGE_EOS). */

  /* Playback end. Handle the message */
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

  /* Seek to start and flush all old data */
  gst_element_seek_simple (pipeline, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, 0);

  /* Clean up nicely */
  gst_element_release_request_pad (tee, req_pad_1);
  gst_element_release_request_pad (tee, req_pad_2);
  gst_object_unref (req_pad_1);
  gst_object_unref (req_pad_2);

  g_print ("Returned, stopping playback\n");
  gst_element_set_state (pipeline, GST_STATE_CHANGE_READY_TO_NULL);

  g_print ("Deleting pipeline\n");
  gst_object_unref (GST_OBJECT (pipeline));

  return 0;
}
