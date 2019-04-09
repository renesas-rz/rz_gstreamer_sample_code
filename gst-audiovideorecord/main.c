#include <gst/gst.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BITRATE_OMXH264ENC 10485760 /* Target bitrate of the encoder element - omxh264enc */
#define BITRATE_ALSASRC    128000      /* Target bitrate of the encoder element - alsasrc */
#define SAMPLE_RATE        44100  			/* Sample rate  of audio file*/
#define CHANNEL            1          			/* Channel*/
#define WIDTH_SIZE         640             /* The output data of v4l2src in this application will be a raw video with 640x480 size */
#define HEIGHT_SIZE        480 
#define F_NV12             "NV12"
#define F_F32LE            "F32LE"
#define OUTPUT_FILE        "/home/media/videos/RECORD_Multimedia.mkv"


#define COMMAND_GET_INPUT_RESOLUTION    "media-ctl -d /dev/media0 --get-v4l2 \"\'adv748x 0-0070 hdmi\':1\" > /home/media/input_resolution.txt"
#define INPUT_RESOLUTION_FILE           "/home/media/input_resolution.txt"

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
  }
}

static void
link_to_multiplexer (GstPad * tolink_pad, GstElement * mux)
{
  GstPad *pad;
  gchar *srcname, *sinkname;

  srcname = gst_pad_get_name (tolink_pad);
  pad = gst_element_get_compatible_pad (mux, tolink_pad, NULL);
  if (gst_pad_link (tolink_pad, pad) != GST_PAD_LINK_OK) {
    g_print ("cannot link request pad\n");
  }
  sinkname = gst_pad_get_name (pad);
  gst_object_unref (GST_OBJECT (pad));

  g_print ("A new pad %s was created and linked to %s\n", sinkname, srcname);
  g_free (sinkname);
  g_free (srcname);
}

int
main (int argc, char *argv[])
{
  GstElement *cam_src, *cam_queue, *cam_capsfilter, *video_converter,
      *video_conv_capsfilter, *video_encoder, *video_parser, *audio_src,
      *audio_queue, *audio_converter, *audio_conv_capsfilter, *audio_encoder,
      *muxer, *sink;

  GstBus *bus;
  GstMessage *msg;
  GstPad *srcpad;
  GstCaps *cam_caps, *video_conv_caps, *audio_conv_caps;

  const gchar *output_file = OUTPUT_FILE;

  if (argv[1] == NULL) {
    g_print ("No input! Please input video device for this app\n");
    return -1;
  }

  int width, height;
  get_input_resolution(&width, &height);
  g_printerr("Input resolution is: %dx%d\n", width, height);
  /* Initialization */
  gst_init (&argc, &argv);

  /* Create GStreamer elements */
  pipeline = gst_pipeline_new ("audio-video-record");

  /* Video elements */
  cam_src = gst_element_factory_make ("v4l2src", "cam-src");
  cam_queue = gst_element_factory_make ("queue", "cam-queue");
  cam_capsfilter = gst_element_factory_make ("capsfilter", "cam_caps");
  video_converter = gst_element_factory_make ("videoconvert", "video-conv");
  video_conv_capsfilter =
      gst_element_factory_make ("capsfilter", "video-conv-caps");
  video_encoder = gst_element_factory_make ("omxh264enc", "video-encoder");
  video_parser = gst_element_factory_make ("h264parse", "h264-parser");

  /* Audio elements */
  audio_src = gst_element_factory_make ("alsasrc", "audio-src");
  audio_queue = gst_element_factory_make ("queue", "audio-queue");
  audio_converter = gst_element_factory_make ("audioconvert", "audio-conv");
  audio_conv_capsfilter =
      gst_element_factory_make ("capsfilter", "audio-conv-caps");
  audio_encoder = gst_element_factory_make ("vorbisenc", "audio-encoder");

  /* container and output elements */
  muxer = gst_element_factory_make ("matroskamux", "mkv-muxer");
  sink = gst_element_factory_make ("filesink", "file-output");

  if (!pipeline || !cam_src || !cam_queue || !cam_capsfilter || !video_converter
      || !video_conv_capsfilter || !video_encoder || !video_parser || !audio_src
      || !audio_queue || !audio_converter || !audio_conv_capsfilter
      || !audio_encoder || !muxer || !sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  /* Set properties */

  /* for video elements */

  /* Set input video device file of the source element - v4l2src */
  g_object_set (G_OBJECT (cam_src), "device", argv[1], NULL);

  /* Set target-bitrate property of the encoder element - omxh264enc */
  g_object_set (G_OBJECT (video_encoder), "target-bitrate", BITRATE_OMXH264ENC, NULL);

  /* for audio elements */

  /* set input device (microphone) of the source element - alsasrc */
  g_object_set (G_OBJECT (audio_src), "device", "hw:0,0", NULL);

  /* set target bitrate of the encoder element - vorbisenc */
  g_object_set (G_OBJECT (audio_encoder), "bitrate", BITRATE_ALSASRC, NULL);

  /* Set output file location of the sink element - filesink */
  g_object_set (G_OBJECT (sink), "location", output_file, NULL);

  /* create simple caps */
  /* RCar-E3 doesn't support input resolution 1920x1080p, so we need add interlace-mode property to capsfilter to limit input resolution to 1920x1080i */
  if(width == 1920 && height == 1080) {
        cam_caps = gst_caps_new_simple("video/x-raw", "interlace-mode", G_TYPE_STRING, "interleaved", "width", G_TYPE_INT, width, "height", G_TYPE_INT, height, NULL);
  } else {
        cam_caps = gst_caps_new_simple("video/x-raw", "width", G_TYPE_INT, width, "height", G_TYPE_INT, height, NULL);
  }

  video_conv_caps =
      gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING, F_NV12,
      NULL);
  audio_conv_caps =
      gst_caps_new_simple ("audio/x-raw",
      "format", G_TYPE_STRING, F_F32LE,
      "channels", G_TYPE_INT, CHANNEL, "rate", G_TYPE_INT, SAMPLE_RATE, NULL);

  /* set caps property for capsfilters */
  g_object_set (G_OBJECT (cam_capsfilter), "caps", cam_caps, NULL);
  g_object_set (G_OBJECT (video_conv_capsfilter), "caps", video_conv_caps,
      NULL);
  g_object_set (G_OBJECT (audio_conv_capsfilter), "caps", audio_conv_caps,
      NULL);

  /* unref caps after usage */
  gst_caps_unref (cam_caps);
  gst_caps_unref (video_conv_caps);
  gst_caps_unref (audio_conv_caps);

  /* Add all elements into the pipeline */
  gst_bin_add_many (GST_BIN (pipeline), cam_src, cam_queue, cam_capsfilter,
      video_converter, video_conv_capsfilter, video_encoder, video_parser,
      audio_src, audio_queue, audio_converter, audio_conv_capsfilter,
      audio_encoder, muxer, sink, NULL);

  /* Link the elements together */
  if (gst_element_link_many (cam_src, cam_queue, cam_capsfilter,
          video_converter, video_conv_capsfilter, video_encoder, video_parser,
          NULL) != TRUE) {
    g_printerr ("Camera record elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }
  if (gst_element_link_many (audio_src, audio_queue, audio_converter,
          audio_conv_capsfilter, audio_encoder, NULL) != TRUE) {
    g_printerr ("Audio record elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }
  if (gst_element_link (muxer, sink) != TRUE) {
    g_printerr ("muxer and sink could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  /* link srcpad of h264parse to request pad of matroskamux */
  srcpad = gst_element_get_static_pad (video_parser, "src");
  link_to_multiplexer (srcpad, muxer);
  gst_object_unref (srcpad);

  /* link srcpad of vorbisenc to request pad of matroskamux */
  srcpad = gst_element_get_static_pad (audio_encoder, "src");
  link_to_multiplexer (srcpad, muxer);
  gst_object_unref (srcpad);

  /* Set the pipeline to "playing" state */
  g_print ("Now recording, press [Ctrl] + [C] to stop...\n");
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
  g_print ("Returned, stopping recording...\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_print ("Deleting pipeline...\n");
  gst_object_unref (GST_OBJECT (pipeline));
  g_print ("Succeeded. Recorded file available at: %s\n", output_file);
  return 0;
}
