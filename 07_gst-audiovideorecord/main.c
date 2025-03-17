/* Copyright (c) 2025 Renesas Electronics Corporation and/or its affiliates */
/* SPDX-License-Identifier: MIT-0 */

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

static GstElement *pipeline;

enum camera_type {
  NO_CAMERA,
  MIPI_CAMERA,
  USB_CAMERA
};

typedef struct tag_user_data
{
  GstElement *pipeline;
  GstElement *cam_src;
  GstElement *cam_queue;
  GstElement *cam_capsfilter;
  GstElement *video_converter;
  GstElement *video_conv_capsfilter;
  GstElement *video_encoder;
  GstElement *video_parser;
  GstElement *audio_src;
  GstElement *audio_queue;
  GstElement *audio_converter;
  GstElement *audio_conv_capsfilter;
  GstElement *audio_encoder;
  GstElement *muxer;
  GstElement *filesink;

  const gchar *device_camera;
  const gchar *device_microphone;
  enum  camera_type camera;
  int width;
  int height;
} UserData;

/* Supported resolutions of MIPI camera */
const char *mipi_resolutions[] = {
  "640x480",
  "1280x720",
  "1920x1080",
  NULL,
};

/* Supported resolutions of USB camera */
const char *usb_resolutions[] = {
  "320x240",
  "640x480",
  "800x600",
  "960x720",
  "1280x720",
  NULL,
};

void
signalHandler (int signal)
{
  if (signal == SIGINT) {
    gst_element_send_event (pipeline, gst_event_new_eos ());
  }
}

static int
link_to_muxer (GstElement *up_element, GstElement *muxer)
{
  gchar *src_name, *sink_name;
  GstPad *src_pad, *req_pad;

  src_pad = gst_element_get_static_pad (up_element, "src");
  src_name = gst_pad_get_name (src_pad);
  req_pad = gst_element_get_compatible_pad (muxer, src_pad, NULL);
  if (!req_pad) {
    g_print ("Request pad can not be found.\n");
    return FALSE;
  }
  sink_name = gst_pad_get_name (req_pad);
  /* Link the source pad and the new request pad */
  gst_pad_link (src_pad, req_pad);
  g_print ("Request pad %s was created and linked to %s\n", sink_name, src_name);

  gst_object_unref (GST_OBJECT (src_pad));
  gst_object_unref (GST_OBJECT (req_pad));
  g_free (src_name);
  g_free (sink_name);

  return TRUE;
}

void
set_element_properties (UserData *data)
{
  GstCaps *camera_caps, *video_convert_caps, *audio_convert_caps;

  if (data->camera == MIPI_CAMERA) {
    /* Set property "dmabuf-use" of vspmfilter to true */
    /* Without it, waylandsink will display broken video */
    g_object_set (G_OBJECT (data->video_converter), "dmabuf-use", true, NULL);
    /* Set properties of the encoder element - omxh264enc */
    g_object_set (G_OBJECT (data->video_encoder),
        "target-bitrate", MIPI_BITRATE_OMXH264ENC,
        "control-rate", VARIABLE_RATE, "interval_intraframes", 14,
        "periodicty-idr", 2, NULL);

    /* Create MIPI camera caps */
    camera_caps =
        gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING, "UYVY",
            "width", G_TYPE_INT, data->width, "height", G_TYPE_INT,
            data->height, NULL);
  } else {
    /* Set properties of the encoder element - omxh264enc */
    g_object_set (G_OBJECT (data->video_encoder),
        "target-bitrate", USB_BITRATE_OMXH264ENC, "control-rate",
        VARIABLE_RATE, NULL);

    /* Create USB camera caps */
    camera_caps =
        gst_caps_new_simple ("video/x-raw", "width", G_TYPE_INT, data->width,
            "height", G_TYPE_INT, data->height, NULL);
  }

  /* Set input video device file of the source element - v4l2src */
  g_object_set (G_OBJECT (data->cam_src), "device", data->device_camera, NULL);

  /* set input device (microphone) of the source element - alsasrc */
  g_object_set (G_OBJECT (data->audio_src), "device", data->device_microphone, NULL);

  /* set target bitrate of the encoder element - vorbisenc */
  g_object_set (G_OBJECT (data->audio_encoder), "bitrate", BITRATE_ALSASRC, NULL);

  /* Set output file location of the filesink element - filesink */
  g_object_set (G_OBJECT (data->filesink), "location", OUTPUT_FILE, NULL);

  /* Create video convert caps */
  video_convert_caps =
      gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING, F_NV12,
          NULL);

  /* Create audio convert caps */
  audio_convert_caps =
      gst_caps_new_simple ("audio/x-raw",
      "format", G_TYPE_STRING, F_F32LE,
      "channels", G_TYPE_INT, CHANNEL, "rate", G_TYPE_INT, SAMPLE_RATE, NULL);

  /* set caps property of capsfilters */
  g_object_set (G_OBJECT (data->cam_capsfilter), "caps", camera_caps, NULL);
  g_object_set (G_OBJECT (data->video_conv_capsfilter), "caps", video_convert_caps,NULL);
  g_object_set (G_OBJECT (data->audio_conv_capsfilter), "caps", audio_convert_caps, NULL);

  /* unref caps after usage */
  gst_caps_unref (camera_caps);
  gst_caps_unref (video_convert_caps);
  gst_caps_unref (audio_convert_caps);
}

int
setup_pipeline (UserData *data)
{
  /* set element properties */
  set_element_properties (data);

  /* Add the elements into the pipeline and link them together */
  gst_bin_add_many (GST_BIN (data->pipeline),
      data->cam_src, data->cam_queue, data->cam_capsfilter, data->video_converter,
      data->video_conv_capsfilter, data->video_encoder, data->video_parser,data->audio_src,
      data->audio_queue, data->audio_converter, data->audio_conv_capsfilter, data->audio_encoder,
      data->muxer, data->filesink, NULL);

  /* Link the elements together. */
  if (gst_element_link_many (data->cam_src, data->cam_queue, data->cam_capsfilter,
          data->video_converter, data->video_conv_capsfilter,
          data->video_encoder, data->video_parser, NULL) != TRUE) {
    g_printerr ("Camera record elements could not be linked.\n");
    return FALSE;
  }

  if (gst_element_link_many (data->audio_src, data->audio_queue, data->audio_converter,
          data->audio_conv_capsfilter, data->audio_encoder, NULL) != TRUE) {
    g_printerr ("Audio record elements could not be linked.\n");
    gst_object_unref (pipeline);
    return FALSE;
  }
  if (gst_element_link (data->muxer, data->filesink) != TRUE) {
    g_printerr ("muxer and filesink could not be linked.\n");
    gst_object_unref (pipeline);
    return FALSE;
  }

  /* link source pad of h264parse to request pad of qtmuxer */
  if (link_to_muxer (data->video_parser, data->muxer) != TRUE) {
    g_printerr ("Failed to link to muxer.\n");
    gst_object_unref (pipeline);
    return FALSE;
  }

  /* link source pad of vorbisenc to request pad of oggmux */
  if (link_to_muxer (data->audio_encoder, data->muxer) != TRUE){
    g_printerr ("Failed to link to muxer.\n");
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
  GstBus *bus;
  GstMessage *msg;
  UserData user_data;

  const gchar *output_file = OUTPUT_FILE;

  if (argc != ARG_COUNT) {
    g_print ("Error: Invalid arugments.\n");
    g_print ("Usage: %s <microphone device> <camera device> [width] [height]\n", argv[ARG_PROGRAM_NAME]);
    return -1;
  }

  user_data.camera = check_camera_type (argv[ARG_CAMERA]);
  if (user_data.camera == NO_CAMERA) {
    return -1;
  } else if (user_data.camera == MIPI_CAMERA) {
    user_data.width = MIPI_WIDTH_SIZE;
    user_data.height = MIPI_HEIGHT_SIZE;
  } else {
    user_data.width = USB_WIDTH_SIZE;
    user_data.height = USB_HEIGHT_SIZE;
  }

  /* Parse resolution from program argument */
  if (!get_resolution (argv[ARG_WIDTH], argv[ARG_HEIGHT],
           &user_data.width, &user_data.height, user_data.camera)) {
    return -1;
  }

  /* Initialization */
  gst_init (&argc, &argv);

  /* Create GStreamer elements */
  user_data.pipeline = gst_pipeline_new ("audio-video-record");

  /* Video elements */
  user_data.cam_src = gst_element_factory_make ("v4l2src", "cam-src");
  user_data.cam_queue = gst_element_factory_make ("queue", "cam-queue");
  user_data.cam_capsfilter = gst_element_factory_make ("capsfilter", "cam_caps");
  user_data.video_conv_capsfilter =
      gst_element_factory_make ("capsfilter", "video-conv-caps");
  user_data.video_encoder = gst_element_factory_make ("omxh264enc", "video-encoder");
  user_data.video_parser = gst_element_factory_make ("h264parse", "h264-parser");

  if (user_data.camera == MIPI_CAMERA) {
    user_data.video_converter = gst_element_factory_make ("vspmfilter", "video-converter");
  } else {
    user_data.video_converter = gst_element_factory_make ("videoconvert", "video-converter");
  }

  /* Audio elements */
  user_data.audio_src = gst_element_factory_make ("alsasrc", "audio-src");
  user_data.audio_queue = gst_element_factory_make ("queue", "audio-queue");
  user_data.audio_converter = gst_element_factory_make ("audioconvert", "audio-conv");
  user_data.audio_conv_capsfilter =
      gst_element_factory_make ("capsfilter", "audio-conv-caps");
  user_data.audio_encoder = gst_element_factory_make ("vorbisenc", "audio-encoder");

  /* container and output elements */
  user_data.muxer = gst_element_factory_make ("matroskamux", "mkv-muxer");
  user_data.filesink = gst_element_factory_make ("filesink", "file-output");

  if (!user_data.pipeline || !user_data.cam_src || !user_data.cam_queue || !user_data.cam_capsfilter || !user_data.video_converter
      || !user_data.video_conv_capsfilter || !user_data.video_encoder || !user_data.video_parser || !user_data.audio_src
      || !user_data.audio_queue || !user_data.audio_converter || !user_data.audio_conv_capsfilter
      || !user_data.audio_encoder || !user_data.muxer || !user_data.filesink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  user_data.device_camera = argv[ARG_CAMERA];
  user_data.device_microphone = argv[ARG_MICROPHONE];

  if (setup_pipeline (&user_data) != TRUE) {
    gst_object_unref (user_data.pipeline);
    return -1;
  }

  /* Set the pipeline to "playing" state */
  g_print ("Now recording, press [Ctrl] + [C] to stop...\n");
  if (gst_element_set_state (user_data.pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (user_data.pipeline);
    return -1;
  }

  /* Handle signals gracefully. */
  pipeline = user_data.pipeline;
  signal (SIGINT, signalHandler);

  /* Wait until error or EOS */
  bus = gst_element_get_bus (user_data.pipeline);
  msg =
      gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
      GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
  gst_object_unref (bus);

  /* Note that because input timeout is GST_CLOCK_TIME_NONE,
     the gst_bus_timed_pop_filtered() function will block forever until a
     matching message was posted on the bus (GST_MESSAGE_ERROR or
     GST_MESSAGE_EOS). */

  if (msg != NULL) {
    parse_message (msg);
    gst_message_unref (msg);
  }

  /* Clean up nicely */
  g_print ("Returned, stopping recording...\n");
  gst_element_set_state (user_data.pipeline, GST_STATE_NULL);

  g_print ("Deleting pipeline...\n");
  gst_object_unref (GST_OBJECT (user_data.pipeline));
  g_print ("Succeeded. Please check output file: %s\n", output_file);
  return 0;
}