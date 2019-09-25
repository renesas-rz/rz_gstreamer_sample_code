#include <gst/gst.h>
#include <stdbool.h>
#include <stdio.h>

#define FORMAT           "S16LE"
#define INPUT_VIDEO_FILE "/home/media/videos/vga1.h264"
#define INPUT_AUDIO_FILE "/home/media/audios/Rondo_Alla_Turka.ogg"

typedef struct _CustomData
{
  GMainLoop *loop;
  int loop_reference;
  GMutex mutex;
} CustomData;

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

static void
on_pad_added (GstElement * element, GstPad * pad, gpointer data)
{
  GstPad *sinkpad;
  GstElement *decoder = (GstElement *) data;

  /* We can now link this pad with the vorbis-decoder sink pad */
  g_print ("Dynamic pad created, linking demuxer/decoder\n");

  sinkpad = gst_element_get_static_pad (decoder, "sink");

  gst_pad_link (pad, sinkpad);

  gst_object_unref (sinkpad);
}

guint
create_audio_pipeline (GstElement ** p_audio_pipeline, const gchar * input_file,
    CustomData * data)
{
  GstBus *bus;
  guint audio_bus_watch_id;
  GstElement *audio_source, *audio_demuxer, *audio_decoder, *audio_converter,
      *audio_capsfilter, *audio_sink;
  GstCaps *capsfilter;

  /* Create GStreamer elements for audio play */
  *p_audio_pipeline = gst_pipeline_new ("audio-play");
  audio_source = gst_element_factory_make ("filesrc", "audio-file-source");
  audio_demuxer = gst_element_factory_make ("oggdemux", "ogg-demuxer");
  audio_decoder = gst_element_factory_make ("vorbisdec", "vorbis-decoder");
  audio_converter = gst_element_factory_make ("audioconvert", "converter");
  audio_capsfilter = gst_element_factory_make ("capsfilter", "convert_caps");
  audio_sink = gst_element_factory_make ("alsasink", "audio-output");

  if (!*p_audio_pipeline || !audio_source || !audio_demuxer || !audio_decoder
      || !audio_converter || !audio_capsfilter || !audio_sink) {
    g_printerr ("One audio element could not be created. Exiting.\n");
    return 0;
  }

  /* Adding a message handler for audio pipeline */
  bus = gst_pipeline_get_bus (GST_PIPELINE (*p_audio_pipeline));
  audio_bus_watch_id = gst_bus_add_watch (bus, bus_call, data);
  gst_object_unref (bus);

  /* Add all elements into the audio pipeline */
  /* file-source | ogg-demuxer | vorbis-decoder | converter | capsfilter | alsa-output */
  gst_bin_add_many (GST_BIN (*p_audio_pipeline),
      audio_source, audio_demuxer, audio_decoder, audio_converter,
      audio_capsfilter, audio_sink, NULL);

  /* Set up for the audio pipeline */
  /* Set the input file location of the file source element */
  g_object_set (G_OBJECT (audio_source), "location", input_file, NULL);

  /* we set the caps option to the caps-filter element */
  capsfilter =
      gst_caps_new_simple ("audio/x-raw", "format", G_TYPE_STRING, FORMAT,
      NULL);
  g_object_set (G_OBJECT (audio_capsfilter), "caps", capsfilter, NULL);
  gst_caps_unref (capsfilter);

  /* we link the elements together */
  /* file-source -> ogg-demuxer ~> vorbis-decoder -> converter -> capsfilter -> alsa-output */
  if (!gst_element_link (audio_source, audio_demuxer)) {
    g_printerr ("Audio elements could not be linked.\n");
    gst_object_unref (*p_audio_pipeline);
    return 0;
  }

  if (!gst_element_link_many (audio_decoder, audio_converter, audio_capsfilter,
          audio_sink, NULL)) {
    g_printerr ("Audio elements could not be linked.\n");
    gst_object_unref (*p_audio_pipeline);
    return 0;
  }

  g_signal_connect (audio_demuxer, "pad-added", G_CALLBACK (on_pad_added),
      audio_decoder);

  return audio_bus_watch_id;
}

guint
create_video_pipeline (GstElement ** p_video_pipeline, const gchar * input_file,
    CustomData * data)
{
  GstBus *bus;
  guint video_bus_watch_id;
  GstElement *video_source, *video_parser, *video_decoder, *video_sink;

  /* Create GStreamer elements for video play */
  *p_video_pipeline = gst_pipeline_new ("video-play");
  video_source = gst_element_factory_make ("filesrc", "video-file-source");
  video_parser = gst_element_factory_make ("h264parse", "h264-parser");
  video_decoder = gst_element_factory_make ("omxh264dec", "h264-decoder");
  video_sink = gst_element_factory_make ("waylandsink", "video-output");

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
  }
  g_mutex_unlock (&p_shared_data->mutex);
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

int
main (int argc, char *argv[])
{
  CustomData shared_data;
  GstElement *audio_pipeline;
  GstElement *video_pipeline;
  guint audio_bus_watch_id;
  guint video_bus_watch_id;
  const gchar *input_audio_file = INPUT_AUDIO_FILE;
  const gchar *input_video_file = INPUT_VIDEO_FILE;

  /* Check input file */
  if (!is_file_exist(input_audio_file) || !is_file_exist(input_video_file))
  {
    g_printerr("Make sure the following files exist:\n");
    g_printerr("  %s\n", input_audio_file);
    g_printerr("  %s\n", input_video_file);

    return -1;
  }

  /* Initialization */
  gst_init (&argc, &argv);
  shared_data.loop = g_main_loop_new (NULL, FALSE);
  shared_data.loop_reference = 0;
  g_mutex_init (&shared_data.mutex);

  /* Create pipelines */
  audio_bus_watch_id =
      create_audio_pipeline (&audio_pipeline, input_audio_file, &shared_data);
  video_bus_watch_id =
      create_video_pipeline (&video_pipeline, input_video_file, &shared_data);

  /* Set the audio pipeline to "playing" state */
  if (audio_bus_watch_id) {
    g_print ("Now playing audio file %s:\n", input_audio_file);
    play_pipeline (audio_pipeline, &shared_data);
  }

  /* Set the video pipeline to "playing" state */
  if (video_bus_watch_id) {
    g_print ("Now playing video file %s:\n", input_video_file);
    play_pipeline (video_pipeline, &shared_data);
  }

  /* Iterate */
  g_print ("Running...\n");
  g_main_loop_run (shared_data.loop);

  /* Out of loop. Clean up nicely */
  g_source_remove (audio_bus_watch_id);
  g_source_remove (video_bus_watch_id);
  g_main_loop_unref (shared_data.loop);
  g_mutex_clear (&shared_data.mutex);
  g_print ("Program end!\n");

  return 0;
}
