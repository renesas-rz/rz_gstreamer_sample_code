#include <gst/gst.h>
#include <pthread.h>
#include "player.h"             /* player UI APIs */

#define SKIP_POSITION               (gint64) 5000000000       /* 5s */
#define NORMAL_PLAYING_RATE         (gdouble) 1.0

#define GET_SECOND_FROM_NANOSEC(x)	(x / 1000000000)
#define AUDIO_SAMPLE_RATE           44100
#define TIME                        60
typedef struct tag_user_data
{
  GMainLoop *loop;
  GstElement *pipeline;
  GstElement *source;
  GstElement *parser;
  GstElement *decoder;
  GstElement *audioresample;
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
pthread_mutex_t mutex_gst_data = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_ui_data = PTHREAD_MUTEX_INITIALIZER;

void
sync_to_play_new_file (UserData * data, gboolean refresh_console_message)
{
  pthread_mutex_lock (&mutex_gst_data);
  play_new_file (data, refresh_console_message);
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
        /* Need to call play_new_file from another thread to avoid DEADLOCK */
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
  GstElement *source = data->source;
  GstElement *parser = data->parser;
  GstElement *decoder = data->decoder;
  GstElement *audioresample = data->audioresample;
  GstElement *capsfilter = data->capsfilter;
  GstElement *sink = data->sink;
  gboolean ret __attribute__((__unused__)) = FALSE;

  /* Seek to start and flush all old data */
  gst_element_seek_simple (data->pipeline, GST_FORMAT_TIME,
      GST_SEEK_FLAG_FLUSH, 0);
  gst_element_set_state (data->pipeline, GST_STATE_READY);

  /* wait until the changing is complete */
  gst_element_get_state (data->pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

  /* Keep the element to still exist after removing */
  source = gst_bin_get_by_name (GST_BIN (data->pipeline), "file-source");
  if (NULL != source) {
    ret = gst_bin_remove (GST_BIN (data->pipeline), data->source);
    LOGD ("gst_bin_remove parser from pipeline: %s\n",
        (ret) ? ("SUCCEEDED") : ("FAILED"));
  }
  /* Keep the element to still exist after removing */
  parser = gst_bin_get_by_name (GST_BIN (data->pipeline), "mp3-parser");
  if (NULL != parser) {
    ret = gst_bin_remove (GST_BIN (data->pipeline), data->parser);
    LOGD ("gst_bin_remove parser from pipeline: %s\n",
        (ret) ? ("SUCCEEDED") : ("FAILED"));
  }
  /* Keep the element to still exist after removing */
  decoder = gst_bin_get_by_name (GST_BIN (data->pipeline), "mp3-decoder");
  if (NULL != decoder) {
    ret = gst_bin_remove (GST_BIN (data->pipeline), data->decoder);
    LOGD ("gst_bin_remove decoder from pipeline: %s\n",
        (ret) ? ("SUCCEEDED") : ("FAILED"));
  }
  /* Keep the element to still exist after removing */
  audioresample = gst_bin_get_by_name (GST_BIN (data->pipeline),
                      "audio-resample");
  if (NULL != audioresample) {
    ret = gst_bin_remove (GST_BIN (data->pipeline), data->audioresample);
    LOGD ("gst_bin_remove audioresample from pipeline: %s\n",
        (ret) ? ("SUCCEEDED") : ("FAILED"));
  }
  /* Keep the element to still exist after removing */
  capsfilter = gst_bin_get_by_name (GST_BIN (data->pipeline),
                  "resample_capsfilter");
  if (NULL != capsfilter) {
    ret = gst_bin_remove (GST_BIN (data->pipeline), data->capsfilter);
    LOGD ("gst_bin_remove capsfilter from pipeline: %s\n",
        (ret) ? ("SUCCEEDED") : ("FAILED"));
  }
  /* Keep the element to still exist after removing */
  sink = gst_bin_get_by_name (GST_BIN (data->pipeline), "audio-output");
  if (NULL != sink) {
    ret = gst_bin_remove (GST_BIN (data->pipeline), data->sink);
    LOGD ("gst_bin_remove sink from pipeline: %s\n",
        (ret) ? ("SUCCEEDED") : ("FAILED"));
  }

  /* Update file location */
  g_object_set (G_OBJECT (data->source), "location", get_current_file_path (),
      NULL);

  /* Add the elements into the pipeline and then link them together */
  gst_bin_add_many (GST_BIN (data->pipeline),
      data->source, data->parser, data->decoder, data->audioresample,
      data->capsfilter, data->sink, NULL);
  if (gst_element_link_many (data->source, data->parser, data->decoder,
          data->audioresample, data->capsfilter, data->sink, NULL) != TRUE) {
    g_print ("Elements could not be linked.\n");
    gst_object_unref (data->pipeline);
  }

  print_current_selected_file (refresh_console_message);

  /* Set the pipeline to "playing" state */
  if (gst_element_set_state (data->pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (data->pipeline);
  }
}

int
main (int argc, char *argv[])
{
  /* Check input arguments */
  if (argc != 2) {
    g_printerr ("Usage: %s <Mp3 filename or directory>\n", argv[0]);
    return -1;
  }

  /* Get dir_path and file_path if possible */
  if (!inject_dir_path_to_player (argv[1])) {
    return -1;
  }

  UserData user_data;
  GstCaps *caps;
  GstBus *bus;
  guint bus_watch_id;

  /* Initialization */
  gst_init (NULL, NULL);
  user_data.loop = g_main_loop_new (NULL, FALSE);

  /* Create GStreamer elements */
  user_data.pipeline = gst_pipeline_new ("audio-player");
  user_data.source = gst_element_factory_make ("filesrc", "file-source");
  user_data.parser = gst_element_factory_make ("mpegaudioparse", "mp3-parser");
  user_data.decoder = gst_element_factory_make ("mpg123audiodec",
                          "mp3-decoder");
  user_data.audioresample = gst_element_factory_make ("audioresample",
                                "audio-resample");
  user_data.capsfilter = gst_element_factory_make ("capsfilter",
                             "resample_capsfilter");
  user_data.sink = gst_element_factory_make ("alsasink", "audio-output");
  user_data.audio_length = 0;

  if (!user_data.pipeline || !user_data.source || !user_data.parser ||
          !user_data.decoder || !user_data.audioresample ||
          !user_data.capsfilter || !user_data.sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  /* Set the caps option to the caps-filter element */
  caps =
      gst_caps_new_simple ("audio/x-raw", "rate", G_TYPE_INT,
          AUDIO_SAMPLE_RATE, NULL);
  g_object_set (G_OBJECT (user_data.capsfilter), "caps", caps, NULL);
  gst_caps_unref (caps);

  /* Set up the pipeline */
  /* we add a message handler */
  bus = gst_pipeline_get_bus (GST_PIPELINE (user_data.pipeline));
  bus_watch_id = gst_bus_add_watch (bus, bus_call, &user_data);
  gst_object_unref (bus);

  /* Show the media file list */
  update_file_list ();

  if (try_to_update_file_path ()) {
    sync_to_play_new_file (&user_data, FALSE);
  }

  /* Handle user input thread */
  if (pthread_create (&id_ui_thread, NULL, check_user_command_loop,
          &user_data)) {
    LOGE ("pthread_create failed\n");
    goto exit;
  }

  /* Iterate */
  g_main_loop_run (user_data.loop);

  /* Out of the main loop, clean up nicely */
  g_print ("Returned, stopping playback...\n");
  gst_element_set_state (user_data.pipeline, GST_STATE_NULL);

  g_print ("Wait for UI thread terminated.\n");
  ui_thread_die = TRUE;
  pthread_join (id_ui_thread, NULL);
  if (id_autoplay_thread != 0) {
    g_print ("Wait Autoplay thread terminated.\n");
    pthread_join (id_autoplay_thread, NULL);
  }

exit:
  g_print ("Deleting pipeline...\n");
  gst_object_unref (GST_OBJECT (user_data.pipeline));
  g_source_remove (bus_watch_id);
  g_main_loop_unref (user_data.loop);

  g_print ("Program end!\n");
  return 0;
}
