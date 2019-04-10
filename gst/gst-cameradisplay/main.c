#include <gst/gst.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH_SIZE            640  /* The output data of v4l2src in this application will be a raw video with 640x480 size */
#define HEIGHT_SIZE           480 
#define TRANSFORMATION_TYPE   "BGRA"
#define OUTPUT_TYPE           "NV12"

#define COMMAND_GET_INPUT_RESOLUTION 	"media-ctl -d /dev/media0 --get-v4l2 \"\'adv748x 0-0070 hdmi\':1\" > /home/media/input_resolution.txt"
#define INPUT_RESOLUTION_FILE     	"/home/media/input_resolution.txt"

static void get_input_resolution(int *available_width_screen, int *available_height_screen) {

        /* Initial variables */
        char *line = NULL;
        size_t len, read;
        FILE *fp;
        char width[10], height[10];

        system(COMMAND_GET_INPUT_RESOLUTION);
        fp = fopen(INPUT_RESOLUTION_FILE, "rt");

        if(fp == NULL) {
                g_printerr("Can't open file.\n");
                exit(1);
        } else {
                while((read = getline(&line, &len, fp)) != -1 ) {
                        char* p_start_width   = strstr(line, "/");
                        p_start_width += 1;
                        char* p_end_width     = strstr(line, "x");

                        char* p_start_height  = strstr(line, "x");
                        p_start_height += 1;
                        char* p_end_height    = strstr(line, " f");


                        memset(width, '\0', sizeof(width));
                        strncpy(width, p_start_width, p_end_width - p_start_width);
                        *available_width_screen = atoi(width);

                        memset(height, '\0', sizeof(height));
                        strncpy(height, p_start_height, p_end_height - p_start_height);
                        *available_height_screen = atoi(height);
                }
        }
        fclose(fp);
}


static GstElement *pipeline;

void
signalHandler (int signal)
{
  if (signal == SIGINT) {
    gst_element_send_event (pipeline, gst_event_new_eos ());
    g_print ("\n");
  }
}


int
main (int argc, char *argv[])
{
  GstElement *source, *camera_capsfilter, *converter, *convert_capsfilter, *sink;
  GstBus *bus;
  GstMessage *msg;
  GstCaps *camera_caps, *convert_caps;

  if (argv[1] == NULL) {
    g_print ("No input! Please input video device for this app\n");
    return -1;
  }

  /* Initialization */
  gst_init (&argc, &argv);

  /* Create GStreamer elements */
  pipeline = gst_pipeline_new ("camera-display");
  source = gst_element_factory_make ("v4l2src", "camera-source");

  int width, height;
  get_input_resolution(&width, &height);
  g_printerr("Input resolution is: %dx%d \n", width, height);
  camera_capsfilter = gst_element_factory_make ("capsfilter", "camera_caps");
  converter = gst_element_factory_make("vspfilter", "video-converter");
  convert_capsfilter = gst_element_factory_make("capsfilter", "convert_caps");
  sink = gst_element_factory_make ("waylandsink", "file-output");

  if (!pipeline || !source || !camera_capsfilter || !converter || !convert_capsfilter|| !sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  /* Set input video device file of the source element - v4l2src */
  g_object_set (G_OBJECT (source), "device", argv[1], NULL);


  /* create simple caps */
  /* RCar-E3 doesn't support input resolution 1920x1080p, so we need add interlace-mode property to capsfilter to limit input resolution to 1920x1080i */
  if(width == 1920 && height == 1080) {
	camera_caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, OUTPUT_TYPE, "interlace-mode", G_TYPE_STRING, "interleaved", "width", G_TYPE_INT, width, "height", G_TYPE_INT, height, NULL);
  } else {
	camera_caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, OUTPUT_TYPE, "width", G_TYPE_INT, width, "height", G_TYPE_INT, height, NULL);
  }


  convert_caps =
      gst_caps_new_simple("video/x-raw", "width", G_TYPE_INT, WIDTH_SIZE, "height", G_TYPE_INT, HEIGHT_SIZE, NULL );

  /* set caps property for capsfilters */
  g_object_set (G_OBJECT (camera_capsfilter),  "caps", camera_caps,	 NULL);
  g_object_set (G_OBJECT (convert_capsfilter), "caps", convert_caps, 	 NULL);

  /* unref caps after usage */
  gst_caps_unref(camera_caps);
  gst_caps_unref(convert_caps);

  /* Add all elements into the pipeline */
  gst_bin_add_many (GST_BIN (pipeline), source, camera_capsfilter, converter, convert_capsfilter, sink, NULL);

  /* Link the elements together */
  if (gst_element_link_many (source, camera_capsfilter, converter, convert_capsfilter, sink, NULL) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  /* Set the pipeline to "playing" state */
  g_print ("Now displaying, press [Ctrl] + [C] to stop...\n");
  if (gst_element_set_state (pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  /* Handle signals gracefully. */
  signal (SIGINT, signalHandler);

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
  g_print ("Returned, stopping display...\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_print ("Deleting pipeline...\n");
  gst_object_unref (GST_OBJECT (pipeline));
  return 0;
}
