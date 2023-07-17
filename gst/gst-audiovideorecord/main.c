#include <gst/gst.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#define MIPI_BITRATE_OMXH264ENC 40000000 /* Target bitrate of the encoder for MIPI camera */
#define USB_BITRATE_OMXH264ENC  10485760 /* Target bitrate of the encoder for USB camera */
#define BITRATE_ALSASRC    128000      /* Target bitrate of the encoder element - alsasrc */
#define SAMPLE_RATE        48000  			/* Sample rate  of audio file*/
#define CHANNEL            1          			/* Channel*/
#define USB_WIDTH_SIZE         1280         /* The output data of v4l2src in this application will be */
#define USB_HEIGHT_SIZE        720          /* a raw video with 1280x720 size */
#define MIPI_WIDTH_SIZE    1280             /* The output data of v4l2src in this application will be */
#define MIPI_HEIGHT_SIZE   960              /* a raw video with 1280x960 size */
#define F_NV12             "NV12"
#define F_F32LE            "F32LE"
#define VARIABLE_RATE      1
#define OUTPUT_FILE        "RECORD_Multimedia.mkv"
#define ARG_PROGRAM_NAME   0
#define ARG_MICROPHONE     1
#define ARG_CAMERA         2
#define ARG_WIDTH          3
#define ARG_HEIGHT         4
#define ARG_COUNT          5

enum camera_type {
  NO_CAMERA,
  MIPI_CAMERA,
  USB_CAMERA
};

/* Supported resolutions of MIPI camera */
const char *mipi_resolutions[] = {
  "1280x960",
  "1920x1080",
  NULL,
};

/* Supported resolutions of USB camera */
const char *usb_resolutions[] = {
  "320x240",
  "640x480",
  "800x600",
  "1280x720",
  NULL,
};

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

/* Check type of camera
 * return NO_CAMERA: Unsupported camera
 * return MIPI_CAMERA: MIPI camera detected
 * return USB_CAMERA: USB camera detected */
enum camera_type
check_camera_type (const char *device)
{
  int fd = -1;
  int ret = -1;
  enum camera_type camera = NO_CAMERA;
  struct v4l2_capability info;

  fd = open (device, O_RDONLY);
  if (fd < 0) {
    g_print ("Cannot open device.\n");
    return NO_CAMERA;
  }

  /* Obtain information about driver */
  ret = ioctl(fd, VIDIOC_QUERYCAP, &info);
  if (ret < 0) {
    g_print ("Invalid V4L2 device.\n");
    close (fd);
    return NO_CAMERA;
  }

  /* Detecting type of camera base on driver*/
  if (strstr ((char*) info.driver, "rzg2l_cru")) {
    g_print ("MIPI camera detected.\n");
    camera = MIPI_CAMERA;
  } else if (strstr ((char*) info.driver, "uvc")) {
    g_print ("USB camera detected.\n");
    camera = USB_CAMERA;
  } else {
    g_print ("Unsupported camera.\n");
    camera = NO_CAMERA;
  }

  close (fd);
  return camera;
}

/* Print supported resolutions in console*/
void
print_supported_resolutions (char *resolution,
    const char* supported_resolutions[]) {
  int index = 0;
  g_print ("%s is unsupported resolution.\n", resolution);
  g_print ("Please try one of the following resolutions:\n");

  /* Print list of supported resolutions */
  while (supported_resolutions[index]) {
    g_print ("%s\n", supported_resolutions[index]);
    index++;
  }
}

/* Check resolution in program argument is supported or not
 * Supported resolutions are defined in
 * usb_resolutions and mipi_resolutions */
bool
check_resolution (char *resolution, const char *supported_resolutions[]) {
  int index = 0;
  bool ret = false;

  while (supported_resolutions[index] != NULL) {
    if (strcmp (supported_resolutions[index], resolution) == 0) {
      ret = true;
      break;
    } else {
      index++;
    }
  }

  if (!ret) {
    print_supported_resolutions (resolution, supported_resolutions);
  }

  return ret;
}

/* Check resolution in program argument is supported or not
 * If not, display list of supported resolutions in console
 * else store resolution to width, height */
bool
get_resolution (char *arg_width, char *arg_height, int *width,
    int *height, enum camera_type camera) {
  bool ret = false;
  char resolution[10];

  sprintf (resolution, "%sx%s", arg_width, arg_height);
  if (camera == MIPI_CAMERA) {
    ret = check_resolution (resolution, mipi_resolutions);
  } else {
    ret = check_resolution (resolution, usb_resolutions);
  }

  if (!ret) {
    return ret;
  }

  /* Store resolution to width and height */
  *width = atoi (arg_width);
  *height = atoi (arg_height);

  return ret;
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
  enum camera_type camera = NO_CAMERA;
  int width = 0;
  int height = 0;

  const gchar *output_file = OUTPUT_FILE;

  if (argc != ARG_COUNT) {
    g_print ("Error: Invalid arugments.\n");
    g_print ("Usage: %s <microphone device> <camera device> [width] [height]\n", argv[ARG_PROGRAM_NAME]);
    return -1;
  }

  camera = check_camera_type (argv[ARG_CAMERA]);
  if (camera == NO_CAMERA) {
    return -1;
  } else if (camera == MIPI_CAMERA) {
    width = MIPI_WIDTH_SIZE;
    height = MIPI_HEIGHT_SIZE;
  } else {
    width = USB_WIDTH_SIZE;
    height = USB_HEIGHT_SIZE;
  }

  /* Parse resolution from program argument */
  if (!get_resolution (argv[ARG_WIDTH], argv[ARG_HEIGHT],
           &width, &height, camera)) {
    return -1;
  }

  /* Initialization */
  gst_init (&argc, &argv);

  /* Create GStreamer elements */
  pipeline = gst_pipeline_new ("audio-video-record");

  /* Video elements */
  cam_src = gst_element_factory_make ("v4l2src", "cam-src");
  cam_queue = gst_element_factory_make ("queue", "cam-queue");
  cam_capsfilter = gst_element_factory_make ("capsfilter", "cam_caps");
  video_conv_capsfilter =
      gst_element_factory_make ("capsfilter", "video-conv-caps");
  video_encoder = gst_element_factory_make ("omxh264enc", "video-encoder");
  video_parser = gst_element_factory_make ("h264parse", "h264-parser");

  if (camera == MIPI_CAMERA) {
    video_converter = gst_element_factory_make ("vspmfilter", "video-converter");
  } else {
    video_converter = gst_element_factory_make ("videoconvert", "video-converter");
  }

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

  if (camera == MIPI_CAMERA) {
    /* Set property "dmabuf-use" of vspmfilter to true */
    /* Without it, the output file will be broken video */
    g_object_set (G_OBJECT (video_converter), "dmabuf-use", true, NULL);
    /* Set properties of the encoder element - omxh264enc */
    g_object_set (G_OBJECT (video_encoder), "target-bitrate", MIPI_BITRATE_OMXH264ENC,
        "control-rate", VARIABLE_RATE, "interval_intraframes", 14,
        "periodicty-idr", 2, NULL);

    /* Create camera caps */
    cam_caps =
        gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING, "UYVY",
            "width", G_TYPE_INT, width, "height", G_TYPE_INT, height, NULL);
  } else {
    /* Set properties of the encoder element - omxh264enc */
    g_object_set (G_OBJECT (video_encoder), "target-bitrate", USB_BITRATE_OMXH264ENC,
        "control-rate", VARIABLE_RATE, NULL);

    /* Create camera caps */
    cam_caps =
        gst_caps_new_simple ("video/x-raw", "width", G_TYPE_INT, width,
            "height", G_TYPE_INT, height, NULL);
  }

  /* Set input video device file of the source element - v4l2src */
  g_object_set (G_OBJECT (cam_src), "device", argv[ARG_CAMERA], NULL);

  /* for audio elements */

  /* set input device (microphone) of the source element - alsasrc */
  g_object_set (G_OBJECT (audio_src), "device", argv[ARG_MICROPHONE], NULL);

  /* set target bitrate of the encoder element - vorbisenc */
  g_object_set (G_OBJECT (audio_encoder), "bitrate", BITRATE_ALSASRC, NULL);

  /* Set output file location of the sink element - filesink */
  g_object_set (G_OBJECT (sink), "location", output_file, NULL);

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
  g_print ("Succeeded. Please check output file: %s\n", output_file);
  return 0;
}
