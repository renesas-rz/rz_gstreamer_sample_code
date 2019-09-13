#include <stdio.h>
#include <string.h>
#include <gst/gst.h>
#include <stdlib.h>

#define INPUT_VIDEO_FILE_1         "/home/media/videos/vga1.h264"
#define INPUT_VIDEO_FILE_2         "/home/media/videos/vga2.h264"
#define INPUT_VIDEO_FILE_3         "/home/media/videos/vga3.h264"
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

typedef struct _DisplayData
{
  int x;
  int y;
  int width;
  int height;
} DisplayData;

typedef struct _CustomData
{
  GMainLoop *loop;
  int loop_reference;
  GMutex mutex;
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
try_to_quit_loop (CustomData * p_shared_data)
{
  g_mutex_lock (&p_shared_data->mutex);
  --(p_shared_data->loop_reference);
  if (0 == p_shared_data->loop_reference) {
    g_main_loop_quit ((p_shared_data->loop));
  }
  g_mutex_unlock (&p_shared_data->mutex);
}

static gboolean
bus_call (GstBus * bus, GstMessage * msg, gpointer data)
{
  GstElement *pipeline = (GstElement *) GST_MESSAGE_SRC (msg);
  gchar *pipeline_name = GST_MESSAGE_SRC_NAME (msg);

  switch (GST_MESSAGE_TYPE (msg)) {

    case GST_MESSAGE_EOS:
      g_print ("End-Of-Stream reached.\n");

      g_print ("Stoping %s...\n", pipeline_name);
      gst_element_set_state (pipeline, GST_STATE_NULL);
      g_print ("Deleting %s...\n", pipeline_name);
      gst_object_unref (GST_OBJECT (pipeline));
      try_to_quit_loop ((CustomData *) data);
      break;

    case GST_MESSAGE_ERROR:{
      gchar *debug_info;
      GError *err;
      gst_message_parse_error (msg, &err, &debug_info);
      g_printerr ("Error received from element %s: %s.\n",
          pipeline_name, err->message);
      g_printerr ("Debugging information: %s.\n",
          debug_info ? debug_info : "none");
      g_clear_error (&err);
      g_free (debug_info);

      g_print ("Stoping %s...\n", pipeline_name);
      gst_element_set_state (pipeline, GST_STATE_NULL);
      g_print ("Deleting %s...\n", pipeline_name);
      gst_object_unref (GST_OBJECT (pipeline));
      try_to_quit_loop ((CustomData *) data);
      break;
    }
    default:
      /* Don't care other message */
      break;
  }
  return TRUE;
}

guint
create_video_pipeline (GstElement ** p_video_pipeline, const gchar * input_file,
    DisplayData * display, CustomData * data)
{
  GstBus *bus;
  guint video_bus_watch_id;
  GstElement *video_source, *video_parser, *video_decoder, *video_sink;

  /* Create GStreamer elements for video play */
  *p_video_pipeline = gst_pipeline_new (NULL);
  video_source = gst_element_factory_make ("filesrc", NULL);
  video_parser = gst_element_factory_make ("h264parse", NULL);
  video_decoder = gst_element_factory_make ("omxh264dec", NULL);
  video_sink = gst_element_factory_make ("waylandsink", NULL);

  if (!*p_video_pipeline || !video_source || !video_parser || !video_decoder
      || !video_sink) {
    g_printerr ("One video element could not be created. Exiting.\n");
    return 0;
  }

  /* Adding a message handler for video pipeline */
  bus = gst_pipeline_get_bus (GST_PIPELINE (*p_video_pipeline));
  video_bus_watch_id = gst_bus_add_watch (bus, bus_call, data);
  gst_object_unref (bus);

  /* Add all elements into the video pipeline */
  /* file-source | h264-parser | h264-decoder | video-output */
  gst_bin_add_many (GST_BIN (*p_video_pipeline), video_source, video_parser,
      video_decoder, video_sink, NULL);

  /* Set up for the video pipeline */
  /* Set the input file location of the file source element */
  g_object_set (G_OBJECT (video_source), "location", input_file, NULL);
  g_object_set (G_OBJECT (video_sink), "position-x", display->x, "position-y",
      display->y, "out-width", display->width, "out-height", display->height,
      NULL);

  /* Link the elements together */
  /* file-source -> h264-parser -> h264-decoder -> video-output */
  if (!gst_element_link_many (video_source, video_parser, video_decoder,
          video_sink, NULL)) {
    g_printerr ("Video elements could not be linked.\n");
    gst_object_unref (*p_video_pipeline);
    return 0;
  }

  return video_bus_watch_id;
}

void
play_pipeline (GstElement * pipeline, CustomData * p_shared_data)
{
  g_mutex_lock (&p_shared_data->mutex);
  ++(p_shared_data->loop_reference);
  if (gst_element_set_state (pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    --(p_shared_data->loop_reference);
    gst_object_unref (pipeline);
  } else {
    /* wait until the changing is complete */
    gst_element_get_state (pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
  }
  g_mutex_unlock (&p_shared_data->mutex);
}

int
main (int argc, char *argv[])
{
  CustomData shared_data;
  GstElement *video_pipeline_1;
  GstElement *video_pipeline_2;
  GstElement *video_pipeline_3;

  guint video_bus_watch_id_1;
  guint video_bus_watch_id_2;
  guint video_bus_watch_id_3;

  const gchar *input_video_file_1 = INPUT_VIDEO_FILE_1;
  const gchar *input_video_file_2 = INPUT_VIDEO_FILE_2;
  const gchar *input_video_file_3 = INPUT_VIDEO_FILE_3;

  DisplayData display;

  /* Get out-width and out-height for the out video */
  get_resolution_from_weston_info(availbe_width_screen, availbe_height_screen);

  /* Scale width and height of the out video to match with screen*/
  int rate_x = availbe_width_screen[0]/5;
  int rate_y = availbe_height_screen[0]/5;

  /* Initialization */
  gst_init (&argc, &argv);
  shared_data.loop = g_main_loop_new (NULL, FALSE);
  shared_data.loop_reference = 0; 	/* Counter */
  g_mutex_init (&shared_data.mutex);

  /* Create first pipeline */
  display.x = POSITION_X;
  display.y = POSITION_Y;
  display.width = availbe_width_screen[0]/3 + rate_x;
  display.height = availbe_height_screen[0]/3 + rate_y;
  video_bus_watch_id_1 =
      create_video_pipeline (&video_pipeline_1, input_video_file_1,
      &display, &shared_data);

  /* Create second pipeline */
  display.x = availbe_width_screen[0]/3 - rate_x;
  display.y = availbe_height_screen[0]/3 - rate_y;
  display.width = availbe_width_screen[0]/3 + rate_x;
  display.height = availbe_height_screen[0]/3 + rate_y;
  video_bus_watch_id_2 =
      create_video_pipeline (&video_pipeline_2, input_video_file_2,
      &display, &shared_data);

  /* Create third pipeline */
  display.x = availbe_width_screen[0]*2/3 -  rate_x;
  display.y = availbe_height_screen[0]*2/3 - rate_y;
  display.width = availbe_width_screen[0]/3 + rate_x;
  display.height = availbe_height_screen[0]/3 + rate_y;
  video_bus_watch_id_3 =
      create_video_pipeline (&video_pipeline_3, input_video_file_3,
      &display, &shared_data);

  /* Set the video pipeline 1 to "playing" state */
  if (video_bus_watch_id_1) {
    g_print ("Now playing video file %s:\n", input_video_file_1);
    play_pipeline (video_pipeline_1, &shared_data);
  }

  /* Set the video pipeline 2 to "playing" state */
  if (video_bus_watch_id_2) {
    g_print ("Now playing video file %s:\n", input_video_file_2);
    play_pipeline (video_pipeline_2, &shared_data);
  }

  /* Set the video pipeline 3 to "playing" state */
  if (video_bus_watch_id_3) {
    g_print ("Now playing video file %s:\n", input_video_file_3);
    play_pipeline (video_pipeline_3, &shared_data);
  }

  /* Iterate */
  g_print ("Running...\n");
  g_main_loop_run (shared_data.loop);

  /* Out of loop. Clean up nicely */
  g_source_remove (video_bus_watch_id_1);
  g_source_remove (video_bus_watch_id_2);
  g_source_remove (video_bus_watch_id_3);
  g_main_loop_unref (shared_data.loop);
  g_mutex_clear (&shared_data.mutex);
  g_print ("Program end!\n");

  return 0;
}
