#include <stdio.h>
#include <string.h>
#include <gst/gst.h>
#include <pthread.h>
#include "player.h"             /* player UI APIs */

#define LOCATION_FILE_WESTON_INFO  "/home/media/data_westoninfo.txt"
#define COMMAND_GET_WESTON_INFO    "weston-info >& /home/media/data_westoninfo.txt"
#define SUBSTRING_WIDTH            "width: "
#define SUBSTRING_HEIGHT           "height: "
#define SUBSTRING_REFRESH          "refresh"
#define SUBSTRING_FLAGS            "flags: current"
#define POSITION_X                    0
#define POSITION_Y                    0
#define LENGTH_STRING                 10
#define INDEX_WESTON_WIDTH            6
#define INDEX_WESTON_HEIGHT           23
#define SKIP_POSITION (gint64)        5000000000       /* 5s */
#define NORMAL_PLAYING_RATE (gdouble) 1.0
#define GET_SECOND_FROM_NANOSEC(x)	 (x / 1000000000)
#define ONE_MINUTE                    60               /* 1 minute = 60 seconds */

/* Count number of available screens */
int count_screens = 0;
/* Declare variable arrays for resolution of screen */
int availbe_width_screen[10];
int availbe_height_screen[10];

typedef struct tag_user_data
{
  GMainLoop *loop;
  GstElement *pipeline;
  GstElement *source;
  GstElement *demuxer;
  GstElement *audio_queue;
  GstElement *audio_decoder;
  GstElement *audio_sink;
  GstElement *video_queue;
  GstElement *video_parser;
  GstElement *video_decoder;
  GstElement *video_sink;
  gint64 media_length;
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

/* Call back functions */
static void
on_pad_added (GstElement * element, GstPad * pad, gpointer data)
{
  GstPad *sinkpad = NULL;
  GstCaps *new_pad_caps = NULL;
  GstStructure *new_pad_struct = NULL;
  const gchar *new_pad_type = NULL;
  UserData *puser_data = (UserData *) data;

  new_pad_caps = gst_pad_query_caps (pad, NULL);
  new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
  new_pad_type = gst_structure_get_name (new_pad_struct);

  GstState currentState;
  GstState pending;

  LOGD ("Received new pad '%s' from '%s': %s\n", GST_PAD_NAME (pad),
      GST_ELEMENT_NAME (element), new_pad_type);

  pthread_mutex_lock (&mutex_gst_data);

  if (g_str_has_prefix (new_pad_type, "audio")) {
    /* Need to set Gst State to PAUSED before change state from NULL to PLAYING */
    gst_element_get_state(puser_data->audio_queue, &currentState, &pending, GST_CLOCK_TIME_NONE);
    if(currentState == GST_STATE_NULL){
      gst_element_set_state (puser_data->audio_queue, GST_STATE_PAUSED);
    }
    gst_element_get_state(puser_data->audio_decoder, &currentState, &pending, GST_CLOCK_TIME_NONE);
    if(currentState == GST_STATE_NULL){
      gst_element_set_state (puser_data->audio_decoder, GST_STATE_PAUSED);
    }
    gst_element_get_state(puser_data->audio_sink, &currentState, &pending, GST_CLOCK_TIME_NONE);
    if(currentState == GST_STATE_NULL){
      gst_element_set_state (puser_data->audio_sink, GST_STATE_PAUSED);
    }

    /* Add back audio_queue, audio_decoder and audio_sink */
    gst_bin_add_many (GST_BIN (puser_data->pipeline),
        puser_data->audio_queue, puser_data->audio_decoder,
        puser_data->audio_sink, NULL);

    /* Link audio_queue +++ audio_decoder +++ audio_sink */
    if (gst_element_link_many (puser_data->audio_queue,
            puser_data->audio_decoder, puser_data->audio_sink, NULL) != TRUE) {
      g_print
          ("audio_queue, audio_decoder and audio_sink could not be linked.\n");
    }

    /* In case link this pad with the AAC-decoder sink pad */
    sinkpad = gst_element_get_static_pad (puser_data->audio_queue, "sink");
    if (GST_PAD_LINK_OK != gst_pad_link (pad, sinkpad)) {
      g_print ("Audio link failed\n");
    } else {
      LOGD ("Audio pad linked!\n");
    }
    gst_object_unref (sinkpad);
    if (new_pad_caps != NULL)
      gst_caps_unref (new_pad_caps);

    /* Change newly added element to ready state is required */
    gst_element_set_state (puser_data->audio_queue, GST_STATE_PLAYING);
    gst_element_set_state (puser_data->audio_decoder, GST_STATE_PLAYING);
    gst_element_set_state (puser_data->audio_sink, GST_STATE_PLAYING);
  } else if (g_str_has_prefix (new_pad_type, "video")) {
    /* Recreate waylandsink */
    if (NULL == puser_data->video_sink) {
      puser_data->video_sink =
          gst_element_factory_make ("waylandsink", "video-output");
      LOGD ("Re-create gst_element_factory_make video_sink: %s\n",
          (NULL == puser_data->video_sink) ? ("FAILED") : ("SUCCEEDED"));

      /* Set position for displaying (0, 0) */
      g_object_set (G_OBJECT (puser_data->video_sink), "position-x", POSITION_X, "position-y", POSITION_Y, NULL);

      /* Set out-width and out-height for the out video */
      g_object_set (G_OBJECT (puser_data->video_sink), "out-width", availbe_width_screen[0], 
                                                       "out-height", availbe_height_screen[0], NULL);
    }

    /* Need to set Gst State to PAUSED before change state from NULL to PLAYING */
    gst_element_get_state(puser_data->video_queue, &currentState, &pending, GST_CLOCK_TIME_NONE);
    if(currentState == GST_STATE_NULL){
      gst_element_set_state (puser_data->video_queue, GST_STATE_PAUSED);
    }
    gst_element_get_state(puser_data->video_decoder, &currentState, &pending, GST_CLOCK_TIME_NONE);
    if(currentState == GST_STATE_NULL){
      gst_element_set_state (puser_data->video_decoder, GST_STATE_PAUSED);
    }
    gst_element_get_state(puser_data->video_sink, &currentState, &pending, GST_CLOCK_TIME_NONE);
    if(currentState == GST_STATE_NULL){
      gst_element_set_state (puser_data->video_sink, GST_STATE_PAUSED);
    }

    /* Add back video_queue, video_decoder and video_sink */
    gst_bin_add_many (GST_BIN (puser_data->pipeline),
        puser_data->video_queue, puser_data->video_parser, puser_data->video_decoder,
        puser_data->video_sink, NULL);

    /* Link video_queue +++ video_decoder +++ video_sink */
    if (gst_element_link_many (puser_data->video_queue, puser_data->video_parser,
            puser_data->video_decoder, puser_data->video_sink, NULL) != TRUE) {
      g_print ("video_queue and video_decoder could not be linked.\n");
    }

    /* In case link this pad with the omxh264-decoder sink pad */
    sinkpad = gst_element_get_static_pad (puser_data->video_queue, "sink");
    if (GST_PAD_LINK_OK != gst_pad_link (pad, sinkpad)) {
      g_print ("Video link failed\n");
    } else {
      LOGD ("Video pad linked!\n");
    }
    gst_object_unref (sinkpad);
    if (new_pad_caps != NULL)
      gst_caps_unref (new_pad_caps);

    /* Change newly added element to ready state is required */
    gst_element_set_state (puser_data->video_queue, GST_STATE_PLAYING);
    gst_element_set_state (puser_data->video_decoder, GST_STATE_PLAYING);
    gst_element_set_state (puser_data->video_sink, GST_STATE_PLAYING);
  }

  /* Signal the dynamic pad linked */
  pthread_mutex_unlock (&mutex_gst_data);
}

static void
no_more_pads (GstElement * element, gpointer data)
{
  pthread_mutex_lock (&mutex_gst_data);
  pthread_cond_signal (&cond_gst_data);
  pthread_mutex_unlock (&mutex_gst_data);
}

/* This function will be call from the UI thread to replay or play next/previous file */
void
sync_to_play_new_file (UserData * data, gboolean refresh_console_message)
{
  pthread_mutex_lock (&mutex_gst_data);
  play_new_file (data, refresh_console_message);
  /* Wait for all pad added - no_more_pads signaled */
  pthread_cond_wait (&cond_gst_data, &mutex_gst_data);
  /* Wait the state become PLAYING to get the video length */
  gst_element_get_state (data->pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
  gst_element_query_duration (data->pipeline, GST_FORMAT_TIME,
      &(data->media_length));
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
  gint64 media_length;
  GstState current_state;

  while (!ui_thread_die) {
    pthread_mutex_lock (&mutex_gst_data);
    pipeline = ((UserData *) data)->pipeline;
    current_state = wait_for_state_change_completed (pipeline);
    media_length = ((UserData *) data)->media_length;
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
        if (pos >= media_length) {
          pos = media_length;
        }
        if (seek_to_time (pipeline, pos)) {
          g_print ("current: %02d:%02d\n",
              (int) (GET_SECOND_FROM_NANOSEC (pos) / ONE_MINUTE),
              (int) (GET_SECOND_FROM_NANOSEC (pos) % ONE_MINUTE));
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
              (int) (GET_SECOND_FROM_NANOSEC (pos) / ONE_MINUTE),
              (int) (GET_SECOND_FROM_NANOSEC (pos) % ONE_MINUTE));
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

/* Seek to beginning and flush all the data in the current pipeline to prepare for the next file */
void
play_new_file (UserData * data, gboolean refresh_console_message)
{
  GstElement *pipeline = data->pipeline;
  GstElement *source = data->source;
  GstElement *video_queue = data->video_queue;
  GstElement *video_decoder = data->video_decoder;
  GstElement *audio_queue = data->audio_queue;
  GstElement *audio_decoder = data->audio_decoder;
  GstElement *audio_sink = data->audio_sink;
  gboolean ret = FALSE;

  GstState currentState;
  GstState pending;

  gst_element_get_state(pipeline, &currentState, &pending, GST_CLOCK_TIME_NONE);
  if(currentState == GST_STATE_PLAYING){
    gst_element_set_state (pipeline, GST_STATE_PAUSED);
    /* Seek to start and flush all old data */
    gst_element_seek_simple (pipeline, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, 0);
  }

  LOGD ("gst_element_set_state pipeline to NULL\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);

  /* wait until the changing is complete */
  gst_element_get_state (pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

  /* Unlink and remove audio elements */
  audio_queue = gst_bin_get_by_name (GST_BIN (pipeline), "audio-queue");        /* Keep the element to still exist after removing */
  if (NULL != audio_queue) {
    ret = gst_bin_remove (GST_BIN (pipeline), data->audio_queue);
    LOGD ("gst_bin_remove audio_queue from pipeline: %s\n",
        (ret) ? ("SUCCEEDED") : ("FAILED"));
  }
  audio_decoder = gst_bin_get_by_name (GST_BIN (pipeline), "aac-decoder");      /* Keep the element to still exist after removing */
  if (NULL != audio_decoder) {
    ret = gst_bin_remove (GST_BIN (pipeline), data->audio_decoder);
    LOGD ("gst_bin_remove audio_decoder from pipeline: %s\n",
        (ret) ? ("SUCCEEDED") : ("FAILED"));
  }
  audio_sink = gst_bin_get_by_name (GST_BIN (pipeline), "audio-output");        /* Keep the element to still exist after removing */
  if (NULL != audio_sink) {
    ret = gst_bin_remove (GST_BIN (pipeline), data->audio_sink);
    LOGD ("gst_bin_remove audio_sink from pipeline: %s\n",
        (ret) ? ("SUCCEEDED") : ("FAILED"));
  }

  /* Unlink and remove video elements */
  video_queue = gst_bin_get_by_name (GST_BIN (pipeline), "video-queue");        /* Keep the element to still exist after removing */
  if (NULL != video_queue) {
    ret = gst_bin_remove (GST_BIN (pipeline), data->video_queue);
    LOGD ("gst_bin_remove video_queue from pipeline: %s\n",
        (ret) ? ("SUCCEEDED") : ("FAILED"));
  }
  video_decoder = gst_bin_get_by_name (GST_BIN (pipeline), "omxh264-decoder");  /* Keep the element to still exist after removing */
  if (NULL != video_decoder) {
    ret = gst_bin_remove (GST_BIN (pipeline), data->video_decoder);
    LOGD ("gst_bin_remove video_decoder from pipeline: %s\n",
        (ret) ? ("SUCCEEDED") : ("FAILED"));
  }

  /* Remove waylandsink completely */
  if (data->video_sink != NULL) {
    ret = gst_bin_remove (GST_BIN (pipeline), data->video_sink);
    LOGD ("gst_bin_remove video_sink from pipeline: %s\n",
        (ret) ? ("SUCCEEDED") : ("FAILED"));
    if (TRUE == ret) {
      data->video_sink = NULL;
    }
  }

  /* Update file location */
  g_object_set (G_OBJECT (source), "location", get_current_file_path (), NULL);

  /* Set the pipeline to "playing" state */
  print_current_selected_file (refresh_console_message);
  gst_element_set_state (pipeline, GST_STATE_PLAYING);
}

int
main (int argc, char *argv[])
{
  /* Check input arguments */
  if (argc != 2) {
    g_printerr ("Usage: %s <MP4 filename or directory>\n", argv[0]);
    return -1;
  }

  /* Get dir_path and file_path if possible */
  if (!inject_dir_path_to_player (argv[1])) {
    return -1;
  }

  GMainLoop *loop;
  UserData user_data;

  GstElement *pipeline, *source, *demuxer;
  GstElement *video_queue, *video_parser, *video_decoder, *video_sink;
  GstElement *audio_queue, *audio_decoder, *audio_sink;

  GstBus *bus;
  guint bus_watch_id;

  /* Get out-width and out-height for the out video */
  get_resolution_from_weston_info(availbe_width_screen, availbe_height_screen);

  /* Initialization */
  gst_init (NULL, NULL);
  loop = g_main_loop_new (NULL, FALSE);

  /* Create GStreamer elements instead of waylandsink */
  pipeline = gst_pipeline_new ("video-player");
  source = gst_element_factory_make ("filesrc", "file-source");
  demuxer = gst_element_factory_make ("qtdemux", "qt-demuxer");
  /* elements for Video thread */
  video_queue = gst_element_factory_make ("queue", "video-queue");
  video_parser = gst_element_factory_make("h264parse", "h264-parser");
  video_decoder = gst_element_factory_make ("omxh264dec", "omxh264-decoder");
  video_sink = NULL;
  /* elements for Audio thread */
  audio_queue = gst_element_factory_make ("queue", "audio-queue");
  audio_decoder = gst_element_factory_make ("faad", "aac-decoder");
  audio_sink = gst_element_factory_make ("alsasink", "audio-output");

  if (!pipeline || !source || !demuxer
      || !video_queue || !video_parser || !video_decoder
      || !audio_queue || !audio_decoder || !audio_sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  /* we add all elements into the pipeline and link them instead of waylandsink */
  gst_bin_add_many (GST_BIN (pipeline), source, demuxer,
      video_queue, video_parser, video_decoder, audio_queue, audio_decoder, audio_sink, NULL);

  /* Link the elements together:
     - file-source -> qt-demuxer
   */
  if (gst_element_link (source, demuxer) != TRUE) {
    g_printerr ("File source could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  /* Construct user data */
  user_data.loop = loop;
  user_data.pipeline = pipeline;
  user_data.source = source;
  user_data.demuxer = demuxer;
  user_data.audio_queue = audio_queue;
  user_data.audio_decoder = audio_decoder;
  user_data.audio_sink = audio_sink;
  user_data.video_queue = video_queue;
  user_data.video_parser = video_parser;
  user_data.video_decoder = video_decoder;
  user_data.video_sink = video_sink;
  user_data.media_length = 0;

  /* Set up the pipeline */
  /* we add a message handler */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, bus_call, &user_data);
  gst_object_unref (bus);

  g_signal_connect (demuxer, "pad-added", G_CALLBACK (on_pad_added),
      &user_data);
  g_signal_connect (demuxer, "no-more-pads", G_CALLBACK (no_more_pads), NULL);

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
