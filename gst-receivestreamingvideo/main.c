#include <stdio.h>
#include <string.h>
#include <gst/gst.h>

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
#define PORT                 5000

/* Count number of available screens */
int count_screens = 0;
/* Declare variable arrays for resolution of screen */
int availbe_width_screen[10];
int availbe_height_screen[10];

static GMainLoop *main_loop;

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

void
signalHandler (int signal)
{
  if (signal == SIGINT) {
    g_main_loop_quit (main_loop);
    g_print ("\n");
  }
}

/* The function bus_call() will be called when a message is received */
static gboolean
bus_call (GstBus * bus, GstMessage * msg, gpointer data)
{
  GMainLoop *loop = (GMainLoop *) data;

  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_ERROR:{
      gchar *debug;
      GError *error;

      gst_message_parse_error (msg, &error, &debug);
      g_free (debug);

      g_printerr ("Error: %s\n", error->message);
      g_error_free (error);

      g_main_loop_quit (loop);
      break;
    }
    default:
      /* Don't care other message */
      break;
  }
  return TRUE;
}

int
main (int argc, char *argv[])
{
  GstElement *pipeline, *source, *depayloader, *parser,
      *decoder, *sink;
  GstCaps *src_caps;
  GstBus *bus;
  guint bus_watch_id;

  /* Get out-width and out-height for the out video */
  get_resolution_from_weston_info(availbe_width_screen, availbe_height_screen);

  /* Initialization */
  gst_init (&argc, &argv);
  main_loop = g_main_loop_new (NULL, FALSE);

  /* Create GStreamer elements */
  pipeline = gst_pipeline_new ("receive-streaming");
  source = gst_element_factory_make ("udpsrc", "udp-src");
  depayloader = gst_element_factory_make ("rtph264depay", "h264-depay");
  parser = gst_element_factory_make ("h264parse", "h264-parser");
  decoder = gst_element_factory_make ("omxh264dec", "h264-decoder");
  sink = gst_element_factory_make ("waylandsink", "video-sink");

  if (!pipeline || !source || !depayloader || !parser
      || !decoder || !sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  bus = gst_element_get_bus (pipeline);
  bus_watch_id = gst_bus_add_watch (bus, bus_call, main_loop);
  gst_object_unref (bus);

  /* For valid RTP packets encapsulated in GstBuffers,
     we use the caps with mime type application/x-rtp.
     Select listening port: 5000 */
  src_caps = gst_caps_new_empty_simple ("application/x-rtp");
  g_object_set (G_OBJECT (source), "port", PORT, "caps", src_caps, NULL);

  /* Set max-lateness maximum number of nanoseconds that a buffer can be late
     before it is dropped (-1 unlimited).
     Generate Quality-of-Service events upstream to FALSE */
  g_object_set (G_OBJECT (sink), "max-lateness", -1, "qos", FALSE, NULL);

  /* Set position for displaying (0, 0) */
  g_object_set (G_OBJECT (sink), "position-x", POSITION_X, "position-y", POSITION_Y, NULL);

  /* Set out-width and out-height for the out video */
  g_object_set (G_OBJECT (sink), "out-width", availbe_width_screen[0], "out-height", availbe_height_screen[0], NULL);

  /* unref cap after use */
  gst_caps_unref (src_caps);

  /* Add all elements into the pipeline */
  gst_bin_add_many (GST_BIN (pipeline), source, depayloader,
      parser, decoder, sink, NULL);

  /* Link the elements together */
  if (gst_element_link_many (source, depayloader, parser,
          decoder, sink, NULL) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  /* Set the pipeline to "playing" state */
  g_print ("Now receiving...\n");
  if (gst_element_set_state (pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  /* Handle signals gracefully. */
  signal (SIGINT, signalHandler);

  /* Iterate */
  g_print ("Running...\n");
  g_main_loop_run (main_loop);

  /* Clean up nicely */
  g_print ("Returned, stopping receiving...\n");
  g_source_remove (bus_watch_id);
  g_main_loop_unref (main_loop);

  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_print ("Deleting pipeline...\n");
  gst_object_unref (GST_OBJECT (pipeline));
  return 0;
}
