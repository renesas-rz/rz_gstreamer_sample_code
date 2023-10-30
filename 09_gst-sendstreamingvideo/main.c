#include <gst/gst.h>
#include <stdbool.h>
#include <stdio.h>
#include <strings.h>
#include <libgen.h>
#include <string.h>
#include <arpa/inet.h>

#define PORT          5000
#define PAYLOAD_TYPE  96
#define TIME          3  /* Send SPS and PPS Insertion every 3 second */

/* Macros for program's arguments */
#define ARG_PROGRAM_NAME 0
#define ARG_IP_ADDRESS   1
#define ARG_INPUT        2
#define ARG_COUNT        3

typedef struct tag_user_data
{
  GstElement *pipeline;
  GstElement *source;
  GstElement *demuxer;
  GstElement *parser;
  GstElement *parser_capsfilter;
  GstElement *payloader;
  GstElement *sink;

  const gchar *input_file;
  const gchar *ip_address;
} UserData;

static void
on_pad_added (GstElement * element, GstPad * pad, gpointer data)
{
  GstPad *sinkpad;
  GstElement *decoder = (GstElement *) data;

  /* We can now link this pad with the H.264-decoder sink pad */
  g_print ("Dynamic pad created, linking demuxer/decoder\n");

  sinkpad = gst_element_get_static_pad (decoder, "sink");

  gst_pad_link (pad, sinkpad);

  gst_object_unref (sinkpad);
}

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

/* get the extension of filename */
const char* get_filename_ext (const char *filename) {
  const char* dot = strrchr (filename, '.');
  if ((!dot) || (dot == filename)) {
    g_print ("Invalid input file.\n");
    return "";
  } else {
    return dot + 1;
  }
}

bool isValidIpAddress(char *ipAddress) {
  struct sockaddr_in sa;
  int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
  return result != 0;
}

void
set_element_properties (UserData *data)
{
  GstCaps *caps;

  /* Set input video device file of the source element - v4l2src */
  g_object_set (G_OBJECT (data->source), "location", data->input_file, NULL);

  /* Send SPS and PPS Insertion every 3 second.
   * https://www.quora.com/What-are-SPS-and-PPS-in-video-codecs
   * For H.264 AVC, dynamic payload is used.
   * https://en.wikipedia.org/wiki/RTP_audio_video_profile
   * http://www.freesoft.org/CIE/RFC/1890/29.htm */

  /* The stream is H.264 AVC so that the payload type must be a dynamic type
   * (96-127).
   * The value 96 is used as a default value */
  g_object_set (G_OBJECT (data->payloader),
      "pt", PAYLOAD_TYPE, "config-interval", TIME, NULL);

  /* Listening to port 5000 of the host */
  g_object_set (G_OBJECT (data->sink),
      "host", data->ip_address, "port", PORT, NULL);

  /* create simple caps */
  caps =
      gst_caps_new_simple ("video/x-h264", "stream-format", G_TYPE_STRING,
      "avc", "alignment", G_TYPE_STRING, "au", NULL);

  /* set caps property for capsfilters */
  g_object_set (G_OBJECT (data->parser_capsfilter), "caps", caps, NULL);

  /* unref caps after usage */
  gst_caps_unref (caps);
}

int
setup_pipeline (UserData *data)
{
  /* set element properties */
  set_element_properties (data);

  /* Add the elements into the pipeline and link them together */
  gst_bin_add_many (GST_BIN (data->pipeline),
      data->source, data->demuxer, data->parser, data->parser_capsfilter,
      data->payloader, data->sink, NULL);
  /* file-src -> converter -> vorbis-encoder */
  if (gst_element_link (data->source, data->demuxer) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    return FALSE;
  }
  /* oggmuxer -> file-output */
  if (gst_element_link_many (data->parser, data->parser_capsfilter,
          data->payloader, data->sink, NULL) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
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

int
main (int argc, char *argv[])
{

  GstBus *bus;
  GstMessage *msg;
  const char* ext;
  char* file_name;
  UserData user_data;

  if (argc != ARG_COUNT) {
    g_print ("Invalid arugments.\n");
    g_print ("Format: %s <IP address> <path to MP4>.\n", argv[ARG_PROGRAM_NAME]);

    return -1;
  }

  if (!isValidIpAddress (argv[ARG_IP_ADDRESS])) {
    g_print ("IP is not valid\n");
    return -1;
  }

  user_data.input_file = argv[ARG_INPUT];
  if (!is_file_exist(user_data.input_file))
  {
    g_printerr("Cannot find input file: %s. Exiting.\n", user_data.input_file);
    return -1;
  }

  file_name = basename ((char*) user_data.input_file);
  ext = get_filename_ext (file_name);

  if (strcasecmp ("mp4", ext) != 0) {
    g_print ("Unsupported video type. MP4 format is required\n");
    return -1;
  }

  /* Initialization */
  gst_init (&argc, &argv);

  /* Create GStreamer elements */
  user_data.pipeline = gst_pipeline_new ("send-streaming");
  user_data.source = gst_element_factory_make ("filesrc", "file-src");
  user_data.demuxer = gst_element_factory_make ("qtdemux", "mp4-demuxer");
  user_data.parser = gst_element_factory_make ("h264parse", "h264-parser");
  user_data.parser_capsfilter =
      gst_element_factory_make ("capsfilter", "parser-capsfilter");
  user_data.payloader = gst_element_factory_make ("rtph264pay",
                            "h264-payloader");
  user_data.sink = gst_element_factory_make ("udpsink", "stream-output");

  if (!user_data.pipeline || !user_data.source || !user_data.demuxer ||
          !user_data.parser || !user_data.parser_capsfilter ||
          !user_data.payloader || !user_data.sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  user_data.ip_address = argv[ARG_IP_ADDRESS];

  if (setup_pipeline (&user_data) != TRUE) {
    gst_object_unref (user_data.pipeline);
    return -1;
  }

  /* Dynamic link */
  g_signal_connect (user_data.demuxer, "pad-added", G_CALLBACK (on_pad_added),
      user_data.parser);

  /* Set the pipeline to "playing" state */
  g_print ("Now streaming: %s...\n", user_data.input_file);
  if (gst_element_set_state (user_data.pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (user_data.pipeline);
    return -1;
  }

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

  g_print ("Returned, stopping streaming...\n");
  gst_element_set_state (user_data.pipeline, GST_STATE_NULL);
  g_print ("Deleting pipeline...\n");
  gst_object_unref (GST_OBJECT (user_data.pipeline));

  return 0;
}
