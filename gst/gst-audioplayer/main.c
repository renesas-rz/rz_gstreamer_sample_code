#include <gst/gst.h>
#include <pthread.h>
#include "player.h"             /* player UI APIs */

#define SKIP_POSITION               (gint64) 5000000000       /* 5s */
#define NORMAL_PLAYING_RATE         (gdouble) 1.0

#define GET_SECOND_FROM_NANOSEC(x)	(x / 1000000000)
#define FORMAT                      "S16LE"
#define TIME                        60
typedef struct tag_user_data
{
  GMainLoop *loop;
  GstElement *pipeline;
  GstElement *source;
  GstElement *demuxer;
  GstElement *decoder;
  GstElement *converter;
  GstElement *capsfilter;
  GstElement *sink;
  gint64 audio_length;
} UserData;

/* Private helper functions */
static gboolean seek_to_time (GstElement * pipeline, gint64 time_nanoseconds);
static gint64 get_current_play_position (GstElement * pipeline);
GstState wait_for_state_change_completed (GstElement * pipeline);
void play_new_file (UserData * data, gboolean refresh_console_message);

/* Local variables */
gboolean ui_thread_die = FALSE;
pthread_t id_ui_thread = 0;
pthread_t id_autoplay_thread = 0;
pthread_cond_t cond_gst_data = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_gst_data = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_ui_data = PTHREAD_MUTEX_INITIALIZER;

/* Call back functions */
static void
on_pad_added (GstElement * element, GstPad * pad, gpointer data)
{
  pthread_mutex_lock (&mutex_gst_data);
  GstPad *sinkpad;
  UserData *puser_data = (UserData *) data;

  LOGD ("inside on_pad_added\n");
  /* Add back audio_queue, audio_decoder and audio_sink */
  gst_bin_add_many (GST_BIN (puser_data->pipeline),
      puser_data->decoder, puser_data->converter, puser_data->capsfilter,
      puser_data->sink, NULL);
  /* Link decoder and converter */
  if (gst_element_link_many (puser_data->decoder, puser_data->converter,
          puser_data->capsfilter, puser_data->sink, NULL) != TRUE) {
    g_print ("Elements could not be linked.\n");
  }

  /* Link demuxer and decoder */
  sinkpad = gst_element_get_static_pad (puser_data->decoder, "sink");
  if (GST_PAD_LINK_OK != gst_pad_link (pad, sinkpad)) {
    g_print ("Link Failed");
  }
  gst_object_unref (sinkpad);

  /* Change newly added element to ready state is required */
  gst_element_set_state (puser_data->decoder, GST_STATE_PLAYING);
  gst_element_set_state (puser_data->converter, GST_STATE_PLAYING);
  gst_element_set_state (puser_data->capsfilter, GST_STATE_PLAYING);
  gst_element_set_state (puser_data->sink, GST_STATE_PLAYING);

  /* Signal the dynamic pad linked */
  pthread_cond_signal (&cond_gst_data);
  pthread_mutex_unlock (&mutex_gst_data);
}

void
sync_to_play_new_file (UserData * data, gboolean refresh_console_message)
{
  pthread_mutex_lock (&mutex_gst_data);
  play_new_file (data, refresh_console_message);
  /* Wait on_pad_added completed in main thread */
  pthread_cond_wait (&cond_gst_data, &mutex_gst_data);
  /* Wait the state become PLAYING to get the audio length */
  gst_element_get_state (data->pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
  gst_element_query_duration (data->pipeline, GST_FORMAT_TIME,
      &(data->audio_length));
  pthread_mutex_unlock (&mutex_gst_data);
}

static void *
auto_play_thread_func (void *data)
{
  sync_to_play_new_file ((UserData *) data, TRUE);
  pthread_exit (NULL);
  return NULL;
}

static gboolean
bus_call (GstBus * bus, GstMessage * msg, gpointer data)
{
  GMainLoop *loop = ((UserData *) data)->loop;
  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_ERROR:{
      gchar *debug;
      GError *error;
      gst_message_parse_error (msg, &error, &debug);
      g_free (debug);
      g_printerr ("Error: %s\n", error->message);
      g_error_free (error);
      fclose (stdin);
      g_main_loop_quit (loop);
      break;
    }
    case GST_MESSAGE_EOS:{
      /* Auto play next file when EOS reach if possible */
      pthread_mutex_lock (&mutex_ui_data);
      gboolean ret = request_update_file_path (1);
      pthread_mutex_unlock (&mutex_ui_data);
      if (ret) {
        /* Need to call play_new_file() from another thread to avoid DEADLOCK */
        if (pthread_create (&id_autoplay_thread, NULL, auto_play_thread_func,
                (UserData *) data)) {
          LOGE ("pthread_create autoplay failed\n");
        }
      }
      break;
    }
    default:
      break;
  }
  return TRUE;
}

static void *
check_user_command_loop (void *data)
{
  GstElement *pipeline;
  gint64 audio_length;
  GstState current_state;

  while (!ui_thread_die) {
    pthread_mutex_lock (&mutex_gst_data);
    pipeline = ((UserData *) data)->pipeline;
    current_state = wait_for_state_change_completed (pipeline);
    audio_length = ((UserData *) data)->audio_length;
    pthread_mutex_unlock (&mutex_gst_data);

    UserCommand cmd = get_user_command ();
    switch (cmd) {
      case QUIT:
        g_main_loop_quit (((UserData *) data)->loop);
        pthread_exit (NULL);
        return NULL;
        break;

      case PAUSE_PLAY:
        if (GST_STATE_PLAYING == current_state) {
          gst_element_set_state (pipeline, GST_STATE_PAUSED);
          g_print ("PAUSED!\n");
        } else if (GST_STATE_PAUSED == current_state) {
          gst_element_set_state (pipeline, GST_STATE_PLAYING);
          g_print ("PLAYED!\n");
        } else {
          g_print ("Cannot PAUSE/PLAY\n");
        }
        break;

      case STOP:{
        /* Seek to the begining */
        if (GST_STATE_PLAYING == current_state) {
          gst_element_set_state (pipeline, GST_STATE_PAUSED);
        }
        if (seek_to_time (pipeline, 0)) {
          g_print ("STOP\n");
        }
        break;
      }

      case REPLAY:
        if (seek_to_time (pipeline, 0)) {
          /* Seek to the begining */
          if (GST_STATE_PAUSED == current_state) {
            gst_element_set_state (pipeline, GST_STATE_PLAYING);
          }
          g_print ("REPLAYED!\n");
        }
        break;

      case FORWARD:{
        gint64 pos = get_current_play_position (pipeline);
        /* calculate the forward position */
        pos = pos + SKIP_POSITION;
        if (pos >= audio_length) {
          pos = audio_length;
        }
        if (seek_to_time (pipeline, pos)) {
          g_print ("current: %02d:%02d\n",
              (int) (GET_SECOND_FROM_NANOSEC (pos) / TIME),
              (int) (GET_SECOND_FROM_NANOSEC (pos) % TIME));
        }
        break;
      }

      case REWIND:{
        gint64 pos = get_current_play_position (pipeline);
        /* calculate the rewind position */
        pos = pos - SKIP_POSITION;
        if (pos < 0) {
          pos = 0;
        }
        if (seek_to_time (pipeline, pos)) {
          g_print ("current: %02d:%02d\n",
              (int) (GET_SECOND_FROM_NANOSEC (pos) / TIME),
              (int) (GET_SECOND_FROM_NANOSEC (pos) % TIME));
        }
        break;
      }

      case LIST:{
        pthread_mutex_lock (&mutex_ui_data);
        update_file_list ();
        pthread_mutex_unlock (&mutex_ui_data);
        break;
      }

      case PREVIOUS:{
        pthread_mutex_lock (&mutex_ui_data);
        gboolean ret = request_update_file_path (-1);
        pthread_mutex_unlock (&mutex_ui_data);
        if (ret) {
          sync_to_play_new_file ((UserData *) data, FALSE);
        }
        break;
      }

      case NEXT:{
        pthread_mutex_lock (&mutex_ui_data);
        gboolean ret = request_update_file_path (1);
        pthread_mutex_unlock (&mutex_ui_data);
        if (ret) {
          sync_to_play_new_file ((UserData *) data, FALSE);
        }
        break;
      }

      case PLAYFILE:{
        pthread_mutex_lock (&mutex_ui_data);
        gboolean ret = request_update_file_path (0);
        pthread_mutex_unlock (&mutex_ui_data);
        if (ret) {
          sync_to_play_new_file ((UserData *) data, FALSE);
        }
        break;
      }

      case HELP:
        print_supported_command ();
        break;

      default:
        break;
    }
  }
  return NULL;
}

/* Private helper functions */
static gboolean
seek_to_time (GstElement * pipeline, gint64 time_nanoseconds)
{
  if (!gst_element_seek (pipeline, NORMAL_PLAYING_RATE, GST_FORMAT_TIME,
          GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, time_nanoseconds,
          GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
    g_print ("Seek failed!\n");
    return FALSE;
  }
  return TRUE;
}

static gint64
get_current_play_position (GstElement * pipeline)
{
  gint64 pos;

  if (gst_element_query_position (pipeline, GST_FORMAT_TIME, &pos)) {
    return pos;
  } else
    return -1;
}

GstState
wait_for_state_change_completed (GstElement * pipeline)
{
  GstState current_state = GST_STATE_VOID_PENDING;
  GstStateChangeReturn state_change_return = GST_STATE_CHANGE_ASYNC;
  do {
    state_change_return =
        gst_element_get_state (pipeline, &current_state, NULL,
        GST_CLOCK_TIME_NONE);
  } while (state_change_return == GST_STATE_CHANGE_ASYNC);
  return current_state;
}

void
play_new_file (UserData * data, gboolean refresh_console_message)
{
  GstElement *decoder = data->decoder;
  GstElement *converter = data->converter;
  GstElement *capsfilter = data->capsfilter;
  GstElement *sink = data->sink;
  gboolean ret = FALSE;

  /* Seek to start and flush all old data */
  gst_element_seek_simple (data->pipeline, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
      0);
  gst_element_set_state (data->pipeline, GST_STATE_READY);

  /* wait until the changing is complete */
  gst_element_get_state (data->pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

  decoder = gst_bin_get_by_name (GST_BIN (data->pipeline), "vorbis-decoder");   /* Keep the element to still exist after removing */
  if (NULL != decoder) {
    ret = gst_bin_remove (GST_BIN (data->pipeline), data->decoder);
    LOGD ("gst_bin_remove decoder from pipeline: %s\n",
        (ret) ? ("SUCCEEDED") : ("FAILED"));
  }
  converter = gst_bin_get_by_name (GST_BIN (data->pipeline), "converter");      /* Keep the element to still exist after removing */
  if (NULL != converter) {
    ret = gst_bin_remove (GST_BIN (data->pipeline), data->converter);
    LOGD ("gst_bin_remove converter from pipeline: %s\n",
        (ret) ? ("SUCCEEDED") : ("FAILED"));
  }
  capsfilter = gst_bin_get_by_name (GST_BIN (data->pipeline), "conv_capsfilter");       /* Keep the element to still exist after removing */
  if (NULL != capsfilter) {
    ret = gst_bin_remove (GST_BIN (data->pipeline), data->capsfilter);
    LOGD ("gst_bin_remove capsfilter from pipeline: %s\n",
        (ret) ? ("SUCCEEDED") : ("FAILED"));
  }
  sink = gst_bin_get_by_name (GST_BIN (data->pipeline), "audio-output");        /* Keep the element to still exist after removing */
  if (NULL != sink) {
    ret = gst_bin_remove (GST_BIN (data->pipeline), data->sink);
    LOGD ("gst_bin_remove sink from pipeline: %s\n",
        (ret) ? ("SUCCEEDED") : ("FAILED"));
  }

  /* Update file location */
  g_object_set (G_OBJECT (data->source), "location", get_current_file_path (),
      NULL);

  /* Set the pipeline to "playing" state */
  print_current_selected_file (refresh_console_message);
  gst_element_set_state (data->pipeline, GST_STATE_PLAYING);
}

int
main (int argc, char *argv[])
{
  /* Check input arguments */
  if (argc != 2) {
    g_printerr ("Usage: %s <Ogg/Vorbis filename or directory>\n", argv[0]);
    return -1;
  }

  /* Get dir_path and file_path if possible */
  if (!inject_dir_path_to_player (argv[1])) {
    return -1;
  }

  GMainLoop *loop;
  UserData user_data;

  GstElement *pipeline, *source, *demuxer, *decoder, *conv, *capsfilter, *sink;
  GstCaps *caps;
  GstBus *bus;
  guint bus_watch_id;

  /* Initialization */
  gst_init (NULL, NULL);
  loop = g_main_loop_new (NULL, FALSE);

  /* Create GStreamer elements instead of demuxer */
  pipeline = gst_pipeline_new ("audio-player");
  source = gst_element_factory_make ("filesrc", "file-source");
  demuxer = gst_element_factory_make ("oggdemux", "ogg-demuxer");
  decoder = gst_element_factory_make ("vorbisdec", "vorbis-decoder");
  conv = gst_element_factory_make ("audioconvert", "converter");
  capsfilter = gst_element_factory_make ("capsfilter", "conv_capsfilter");
  sink = gst_element_factory_make ("alsasink", "audio-output");

  if (!pipeline || !source || !demuxer || !decoder || !conv || !capsfilter
      || !sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  /* Set the caps option to the caps-filter element */
  caps =
      gst_caps_new_simple ("audio/x-raw", "format", G_TYPE_STRING, FORMAT,
      NULL);
  g_object_set (G_OBJECT (capsfilter), "caps", caps, NULL);
  gst_caps_unref (caps);


  /* we add all elements into the pipeline and link them instead of demuxer */
  gst_bin_add_many (GST_BIN (pipeline), source, demuxer, decoder, conv,
      capsfilter, sink, NULL);
  if (gst_element_link (source, demuxer) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }
  if (gst_element_link_many (decoder, conv, capsfilter, sink, NULL) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  /* Construct user data */
  user_data.loop = loop;
  user_data.pipeline = pipeline;
  user_data.source = source;
  user_data.demuxer = demuxer;
  user_data.decoder = decoder;
  user_data.converter = conv;
  user_data.capsfilter = capsfilter;
  user_data.sink = sink;
  user_data.audio_length = 0;

  g_signal_connect (demuxer, "pad-added", G_CALLBACK (on_pad_added),
      &user_data);

  /* Set up the pipeline */
  /* we add a message handler */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, bus_call, &user_data);
  gst_object_unref (bus);

  /* Show the media file list */
  update_file_list ();

  if (try_to_update_file_path ()) {
    sync_to_play_new_file (&user_data, FALSE);
  }

  /* Handle user input thread */
  if (pthread_create (&id_ui_thread, NULL, check_user_command_loop, &user_data)) {
    LOGE ("pthread_create failed\n");
    goto exit;
  }

  /* Iterate */
  g_main_loop_run (loop);

  /* Out of the main loop, clean up nicely */
  g_print ("Returned, stopping playback...\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_print ("Wait for UI thread terminated.\n");
  ui_thread_die = TRUE;
  pthread_join (id_ui_thread, NULL);
  if (id_autoplay_thread != 0) {
    g_print ("Wait Autoplay thread terminated.\n");
    pthread_join (id_autoplay_thread, NULL);
  }

exit:
  g_print ("Deleting pipeline...\n");
  gst_object_unref (GST_OBJECT (pipeline));
  g_source_remove (bus_watch_id);
  g_main_loop_unref (loop);

  g_print ("Program end!\n");
  return 0;
}
