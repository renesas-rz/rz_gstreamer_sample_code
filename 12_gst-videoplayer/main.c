/* Copyright (c) 2023 Renesas Electronics Corporation and/or its affiliates */
/* SPDX-License-Identifier: MIT-0 */

#include <stdio.h>
#include <string.h>
#include <gst/gst.h>
#include <pthread.h>
#include "player.h"             /* player UI APIs */
#include <stdbool.h>
#include <wayland-client.h>

/* 5s */
#define SKIP_POSITION (gint64)        5000000000
#define NORMAL_PLAYING_RATE (gdouble) 1.0
#define GET_SECOND_FROM_NANOSEC(x)	 (x / 1000000000)
/* 1 minute = 60 seconds */
#define ONE_MINUTE                    60
#define AUDIO_SAMPLE_RATE             44100

/* These structs contain information needed to get a list of available
 * screens */
struct screen_t
{
  uint16_t x;
  uint16_t y;

  uint16_t width;
  uint16_t height;

  struct wl_list link;
};

struct wayland_t
{
  struct wl_display *display;
  struct wl_registry *registry;
  struct wl_output *output;

  struct wl_list screens;
};

typedef struct tag_user_data
{
  GMainLoop *loop;
  GstElement *pipeline;
  GstElement *source;
  GstElement *demuxer;
  GstElement *audio_queue;
  GstElement *audio_decoder;
  GstElement *audio_resample;
  GstElement *audio_capsfilter;
  GstElement *audio_sink;
  GstElement *video_queue;
  GstElement *video_parser;
  GstElement *video_decoder;
  GstElement *video_filter;
  GstElement *video_capsfilter;
  GstElement *video_sink;
  gint64 media_length;
  struct screen_t *main_screen;
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
 * name: init_wayland
 * Initialize "wayland_t" structure
 *
 */
void
init_wayland(struct wayland_t *handler)
{
  if (handler != NULL)
  {
    /* Initialize "wayland_t" structure */
    handler->display = NULL;
    handler->registry = NULL;
    handler->output = NULL;

    /* Initialize doubly-linked list */
    wl_list_init(&(handler->screens));
  }
}

/*
 *
 * name: destroy_wayland
 * De-initialize wayland_t structure
 *
 */
void
destroy_wayland(struct wayland_t *handler)
{
  struct screen_t *screen = NULL;
  struct screen_t *tmp = NULL;

  if (handler != NULL)
  {
    /* Clean up screens */
    if (!wl_list_empty(&(handler->screens)))
    {
      wl_list_for_each_safe(screen, tmp, &(handler->screens), link)
      {
        wl_list_remove(&(screen->link));
        free(screen);
      }
    }

    /* Clean up wayland */
    if (handler->output != NULL)
    {
      wl_output_destroy(handler->output);
    }

    if (handler->registry != NULL)
    {
      wl_registry_destroy(handler->registry);
    }

    if (handler->display != NULL)
    {
      wl_display_disconnect(handler->display);
    }

    /* De-allocate "wayland_t" structure itself */
    free(handler);
  }
}

/*
 *
 * name: output_handle_geometry
 * Obtain geometry information, such as: name, model, physical width,
 * physical height...
 *
 */
static void
output_handle_geometry(void *data, struct wl_output *wl_output,
		       int32_t x, int32_t y,
		       int32_t physical_width, int32_t physical_height,
		       int32_t subpixel,
		       const char *make, const char *model,
		       int32_t output_transform)
{
  struct screen_t *screen = (struct screen_t*)data;

  screen->x = x;
  screen->y = y;
}

/*
 *
 * name: output_handle_mode
 * Obtain screen's information, such as: width, height, refresh rate....
 *
 */
static void
output_handle_mode(void *data, struct wl_output *wl_output,
		   uint32_t flags, int32_t width, int32_t height,
		   int32_t refresh)
{
  struct screen_t *screen = (struct screen_t*)data;
  if (flags & WL_OUTPUT_MODE_CURRENT)
  {
    screen->width = width;
    screen->height = height;
  }
}

/*
 *
 * name: output_handle_scale
 * Obtain geometry scale
 *
 */
static void
output_handle_scale(void *data, struct wl_output *wl_output,
		    int32_t scale)
{
  /* Do nothing */
}

static void
output_handle_done(void *data, struct wl_output *wl_output)
{
  /* Do nothing */
}

/* This variable is used to get information from global object "wl_outout" */
static const struct wl_output_listener output_listener =
{
  output_handle_geometry,
  output_handle_mode,
  output_handle_done,
  output_handle_scale,
};

/*
 *
 * name: global_handler
 * Register global objects from Wayland compositor
 *
 */
static void
global_handler(void *data, struct wl_registry *registry, uint32_t id,
	       const char *interface, uint32_t version)
{
  struct screen_t *screen = NULL;
  struct wayland_t *handler = (struct wayland_t*)data;

  if (strcmp(interface, "wl_output") == 0)
  {
    /* Allocate and initialize "screen_t" structure */
    screen = (struct screen_t*)calloc(1, sizeof(struct screen_t));

    if (screen != NULL)
    {
      handler->output = wl_registry_bind(handler->registry, id,
                            &wl_output_interface, MIN(version, 2));
      wl_output_add_listener(handler->output, &output_listener, screen);

      /* Wait until all screen's data members are filled */
      wl_display_roundtrip(handler->display);

      if ((screen->width == 0) || (screen->height == 0))
      {
        /* Remove invalid screen */
        free(screen);
      }
      else
      {
        /* Add this new screen to the head of doubly-linked list */
        wl_list_insert(&(handler->screens), &(screen->link));
      }
    }
  }
}

/*
 *
 * name: global_remove_handler
 * Remove public objects from Wayland compositor
 *
 */
static void
global_remove_handler(void *data, struct wl_registry *registry, uint32_t name)
{
  /* Do nothing */
}

/* This variable contains functions to register public Wayland's objects */
static const struct wl_registry_listener registry_listener =
{
  global_handler,
  global_remove_handler
};

/*
 *
 * name: get_available_screens
 * Get a list of available screens
 *
 */
struct wayland_t*
get_available_screens()
{
  struct wayland_t *handler = calloc(1, sizeof(struct wayland_t));
  if (handler == NULL)
  {
    return NULL;
  }

  /* Initialize "wayland_t" structure */
  init_wayland(handler);

  /* Connect to weston compositor */
  handler->display = wl_display_connect(NULL);
  if (handler->display == NULL)
  {
    fprintf(stderr, "Failed to create display\n");
    free(handler);

    return NULL;
  }

  /* Obtain wl_registry from Wayland compositor to access public object
   * "wl_output" */
  handler->registry = wl_display_get_registry(handler->display);
  wl_registry_add_listener(handler->registry, &registry_listener, handler);

  /* Wait until public object "wl_output" is binded */
  wl_display_roundtrip(handler->display);

  return handler;
}

/*
 *
 * name: get_main_screen
 * Get main screen which has axis (0, 0)
 *
 */
struct screen_t*
get_main_screen(struct wayland_t *handler)
{
  struct screen_t *result = NULL;

  if ((handler != NULL) && !wl_list_empty(&(handler->screens)))
  {
    wl_list_for_each(result, &(handler->screens), link)
    {
      if ((result->x == 0) && (result->y == 0))
      {
        return result;
      }
    }
  }

  return NULL;
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
  struct screen_t *main_screen = puser_data->main_screen;

  new_pad_caps = gst_pad_query_caps (pad, NULL);
  new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
  new_pad_type = gst_structure_get_name (new_pad_struct);

  GstState currentState;
  GstState pending;

  LOGD ("Received new pad '%s' from '%s': %s\n", GST_PAD_NAME (pad),
      GST_ELEMENT_NAME (element), new_pad_type);

  pthread_mutex_lock (&mutex_gst_data);

  if (g_str_has_prefix (new_pad_type, "audio")) {
    /* Need to set Gst State to PAUSED before change state from NULL to
     * PLAYING */
    gst_element_get_state(puser_data->audio_queue, &currentState, &pending,
        GST_CLOCK_TIME_NONE);
    if(currentState == GST_STATE_NULL){
      gst_element_set_state (puser_data->audio_queue, GST_STATE_PAUSED);
    }
    gst_element_get_state(puser_data->audio_decoder, &currentState, &pending,
        GST_CLOCK_TIME_NONE);
    if(currentState == GST_STATE_NULL){
      gst_element_set_state (puser_data->audio_decoder, GST_STATE_PAUSED);
    }
    gst_element_get_state(puser_data->audio_resample, &currentState, &pending,
        GST_CLOCK_TIME_NONE);
    if(currentState == GST_STATE_NULL){
      gst_element_set_state (puser_data->audio_resample, GST_STATE_PAUSED);
    }
    gst_element_get_state(puser_data->audio_capsfilter, &currentState,
        &pending, GST_CLOCK_TIME_NONE);
    if(currentState == GST_STATE_NULL){
      gst_element_set_state (puser_data->audio_capsfilter, GST_STATE_PAUSED);
    }
    gst_element_get_state(puser_data->audio_sink, &currentState, &pending,
        GST_CLOCK_TIME_NONE);
    if(currentState == GST_STATE_NULL){
      gst_element_set_state (puser_data->audio_sink, GST_STATE_PAUSED);
    }

    /* Add back audio_queue, audio_decoder, audio_resample, audio_capsfilter,
     * and audio_sink */
    gst_bin_add_many (GST_BIN (puser_data->pipeline),
        puser_data->audio_queue, puser_data->audio_decoder,
        puser_data->audio_resample, puser_data->audio_capsfilter,
        puser_data->audio_sink, NULL);

    /* Link audio_queue -> audio_decoder -> audio_resample -> audio_capsfilter
     * -> audio_sink */
    if (gst_element_link_many (puser_data->audio_queue,
            puser_data->audio_decoder, puser_data->audio_resample,
            puser_data->audio_capsfilter, puser_data->audio_sink,
            NULL) != TRUE) {
      g_print
          ("audio_queue, audio_decoder, audio_resample, audio_capsfilter, " \
           "and audio_sink could not be linked.\n");
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

    /* Change the pipeline to PLAYING state
     * TODO: The below statement temporarily fixes issue "Unable to pause the
     * video".
     * The root cause is still unknown.
     */
    gst_element_set_state (puser_data->pipeline, GST_STATE_PLAYING);
  } else if (g_str_has_prefix (new_pad_type, "video")) {
    /* Recreate waylandsink */
    if (NULL == puser_data->video_sink) {
      puser_data->video_sink =
          gst_element_factory_make ("waylandsink", "video-output");
      LOGD ("Re-create gst_element_factory_make video_sink: %s\n",
          (NULL == puser_data->video_sink) ? ("FAILED") : ("SUCCEEDED"));

      /* Set position for displaying (0, 0) */
      g_object_set (G_OBJECT (puser_data->video_sink),
          "position-x", main_screen->x, "position-y", main_screen->y, NULL);

    }

    /* Create decoder and parser for H264 video */
    if (g_str_has_prefix (new_pad_type, "video/x-h264")) {
      /* Recreate video-parser */
      if (NULL == puser_data->video_parser) {
        puser_data->video_parser =
            gst_element_factory_make ("h264parse", "h264-parser");
        LOGD ("Re-create gst_element_factory_make video_parser: %s\n",
            (NULL == puser_data->video_parser) ? ("FAILED") : ("SUCCEEDED"));
      }
      /* Recreate video-decoder */
      if (NULL == puser_data->video_decoder) {
        puser_data->video_decoder =
            gst_element_factory_make ("omxh264dec", "omxh264-decoder");
        LOGD ("Re-create gst_element_factory_make video_decoder: %s\n",
            (NULL == puser_data->video_decoder) ? ("FAILED") : ("SUCCEEDED"));
      }
    } else {
      g_print("Unsupported video format\n");
      g_main_loop_quit (puser_data->loop);
    }

    /* Need to set Gst State to PAUSED before change state from NULL to
     * PLAYING */
    gst_element_get_state(puser_data->video_queue, &currentState, &pending,
        GST_CLOCK_TIME_NONE);
    if(currentState == GST_STATE_NULL){
      gst_element_set_state (puser_data->video_queue, GST_STATE_PAUSED);
    }
    gst_element_get_state(puser_data->video_parser, &currentState, &pending,
        GST_CLOCK_TIME_NONE);
    if (currentState == GST_STATE_NULL){
      gst_element_set_state (puser_data->video_parser, GST_STATE_PAUSED);
    }
    gst_element_get_state(puser_data->video_decoder, &currentState, &pending,
        GST_CLOCK_TIME_NONE);
    if(currentState == GST_STATE_NULL){
      gst_element_set_state (puser_data->video_decoder, GST_STATE_PAUSED);
    }
    gst_element_get_state(puser_data->video_sink, &currentState, &pending,
        GST_CLOCK_TIME_NONE);
    if(currentState == GST_STATE_NULL){
      gst_element_set_state (puser_data->video_sink, GST_STATE_PAUSED);
    }

    /* Add back video_queue, video_parser, video_decoder, and video_sink */
    gst_bin_add_many (GST_BIN (puser_data->pipeline),
        puser_data->video_queue, puser_data->video_parser,
        puser_data->video_decoder, puser_data->video_sink, NULL);

    /* Link video_queue -> video_parser -> video_decoder -> video_sink */
    if (gst_element_link_many (puser_data->video_queue,
            puser_data->video_parser, puser_data->video_decoder,
            puser_data->video_sink, NULL) != TRUE) {
      g_print ("video_queue, video_parser, video_decoder and video_sink " \
          "could not be linked.\n");
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

    /* Change the pipeline to PLAYING state
     * TODO: The below statement temporarily fixes issue "Unable to pause the
     * video".
     * The root cause is still unknown.
     */
    gst_element_set_state (puser_data->pipeline, GST_STATE_PLAYING);
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

/* This function will be call from the UI thread to replay or play
 * next/previous file */
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
        /* Need to call play_new_file() from another thread to avoid
         * DEADLOCK */
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

/* Seek to beginning and flush all the data in the current pipeline to prepare
 * for the next file */
void
play_new_file (UserData * data, gboolean refresh_console_message)
{
  GstElement *pipeline = data->pipeline;
  GstElement *source = data->source;
  GstElement *video_queue = data->video_queue;
  GstElement *audio_resample = data->audio_resample;
  GstElement *audio_capsfilter = data->audio_capsfilter;
  GstElement *audio_queue = data->audio_queue;
  GstElement *audio_decoder = data->audio_decoder;
  GstElement *audio_sink = data->audio_sink;
  gboolean ret = FALSE;

  GstState currentState;
  GstState pending;

  gst_element_get_state(pipeline, &currentState, &pending,
      GST_CLOCK_TIME_NONE);
  if(currentState == GST_STATE_PLAYING){
    gst_element_set_state (pipeline, GST_STATE_PAUSED);
    /* Seek to start and flush all old data */
    gst_element_seek_simple (pipeline, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
        0);
  }

  LOGD ("gst_element_set_state pipeline to NULL\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);

  /* wait until the changing is complete */
  gst_element_get_state (pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);

  /* Unlink and remove audio elements */
  /* Keep the element to still exist after removing */
  audio_queue = gst_bin_get_by_name (GST_BIN (pipeline), "audio-queue");
  if (NULL != audio_queue) {
    ret = gst_bin_remove (GST_BIN (pipeline), data->audio_queue);
    LOGD ("gst_bin_remove audio_queue from pipeline: %s\n",
        (ret) ? ("SUCCEEDED") : ("FAILED"));
  }
  /* Keep the element to still exist after removing */
  audio_decoder = gst_bin_get_by_name (GST_BIN (pipeline), "aac-decoder");
  if (NULL != audio_decoder) {
    ret = gst_bin_remove (GST_BIN (pipeline), data->audio_decoder);
    LOGD ("gst_bin_remove audio_decoder from pipeline: %s\n",
        (ret) ? ("SUCCEEDED") : ("FAILED"));
  }
  audio_resample = gst_bin_get_by_name (GST_BIN (pipeline), "audio-resample");
  if (NULL != audio_resample)
  {
    ret = gst_bin_remove (GST_BIN (pipeline), data->audio_resample);
    LOGD ("gst_bin_remove audio_resample from pipeline: %s\n",
        (ret) ? ("SUCCEEDED") : ("FAILED"));
  }
  audio_capsfilter = gst_bin_get_by_name (GST_BIN (pipeline),
                         "audio-capsfilter");
  if (NULL != audio_capsfilter)
  {
    ret = gst_bin_remove (GST_BIN (pipeline), data->audio_capsfilter);
    LOGD ("gst_bin_remove audio_capsfilter from pipeline: %s\n",
        (ret) ? ("SUCCEEDED") : ("FAILED"));
  }
  /* Keep the element to still exist after removing */
  audio_sink = gst_bin_get_by_name (GST_BIN (pipeline), "audio-output");
  if (NULL != audio_sink) {
    ret = gst_bin_remove (GST_BIN (pipeline), data->audio_sink);
    LOGD ("gst_bin_remove audio_sink from pipeline: %s\n",
        (ret) ? ("SUCCEEDED") : ("FAILED"));
  }

  /* Unlink and remove video elements */
  /* Keep the element to still exist after removing */
  video_queue = gst_bin_get_by_name (GST_BIN (pipeline), "video-queue");
  if (NULL != video_queue) {
    ret = gst_bin_remove (GST_BIN (pipeline), data->video_queue);
    LOGD ("gst_bin_remove video_queue from pipeline: %s\n",
        (ret) ? ("SUCCEEDED") : ("FAILED"));
  }

  /* Remove video-parser completely */
  if (data->video_parser != NULL) {
    ret = gst_bin_remove (GST_BIN (pipeline), data->video_parser);
    LOGD ("gst_bin_remove video_parser from pipeline: %s\n",
        (ret) ? ("SUCCEEDED") : ("FAILED"));
    if (TRUE == ret) {
      data->video_parser = NULL;
    }
  }

  /* Remove video-decoder completely */
  if (data->video_decoder != NULL) {
    ret = gst_bin_remove (GST_BIN (pipeline), data->video_decoder);
    LOGD ("gst_bin_remove video_decoder from pipeline: %s\n",
        (ret) ? ("SUCCEEDED") : ("FAILED"));
    if (TRUE == ret) {
     data->video_decoder = NULL;
    }
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
  struct wayland_t *wayland_handler = NULL;
  UserData user_data;

  GstCaps *caps;
  GstBus *bus;
  guint bus_watch_id;

  /* Check input arguments */
  if (argc != 2) {
    g_printerr ("Usage: %s <MP4 filename or directory>\n", argv[0]);
    return -1;
  }

  /* Get a list of available screen */
  wayland_handler = get_available_screens();

  /* Get main screen */
  user_data.main_screen = get_main_screen(wayland_handler);
  if (user_data.main_screen == NULL)
  {
    g_printerr("Cannot find any available screens. Exiting.\n");

    destroy_wayland (wayland_handler);
    return -1;
  }

  destroy_wayland (wayland_handler);

  /* Get dir_path and file_path if possible */
  if (argc == 2) {
    if (!inject_dir_path_to_player (argv[1])) {
      return -1;
    }
  } else if (argc == 3) {
    if (!inject_dir_path_to_player (argv[2])) {
      return -1;
    }
  }

  /* Initialization */
  gst_init (NULL, NULL);
  user_data.loop = g_main_loop_new (NULL, FALSE);

  /* Create GStreamer elements instead of waylandsink */
  user_data.pipeline = gst_pipeline_new ("video-player");
  user_data.source = gst_element_factory_make ("filesrc", "file-source");
  user_data.demuxer = gst_element_factory_make ("qtdemux", "qt-demuxer");
  /* elements for Video thread */
  user_data.video_queue = gst_element_factory_make ("queue", "video-queue");
  user_data.video_sink = NULL;
  /* elements for Audio thread */
  user_data.audio_queue = gst_element_factory_make ("queue", "audio-queue");
  user_data.audio_decoder = gst_element_factory_make ("faad", "aac-decoder");
  user_data.audio_resample = gst_element_factory_make("audioresample",
                                 "audio-resample");
  user_data.audio_capsfilter = gst_element_factory_make ("capsfilter",
                                   "audio-capsfilter");
  user_data.audio_sink = gst_element_factory_make ("alsasink", "audio-output");

  if (!user_data.pipeline || !user_data.source || !user_data.demuxer ||
          !user_data.video_queue || !user_data.audio_queue ||
          !user_data.audio_decoder || !user_data.audio_resample ||
          !user_data.audio_capsfilter || !user_data.audio_sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  /* Create simple cap which contains audio's sample rate */
  caps = gst_caps_new_simple ("audio/x-raw",
      "rate", G_TYPE_INT, AUDIO_SAMPLE_RATE, NULL);

  /* Add cap to capsfilter element */
  g_object_set (G_OBJECT (user_data.audio_capsfilter), "caps", caps, NULL);
  gst_caps_unref (caps);

  /* Add all created elements into the pipeline */
  gst_bin_add_many (GST_BIN (user_data.pipeline),
      user_data.source, user_data.demuxer, user_data.video_queue,
      user_data.audio_queue, user_data.audio_decoder, user_data.audio_resample,
      user_data.audio_capsfilter, user_data.audio_sink, NULL);

  /* Link the elements together:
     - file-source -> qt-demuxer
   */
  if (gst_element_link (user_data.source, user_data.demuxer) != TRUE) {
    g_printerr ("File source could not be linked.\n");
    gst_object_unref (user_data.pipeline);
    return -1;
  }

  user_data.video_parser = NULL;
  user_data.video_decoder = NULL;
  user_data.media_length = 0;

  /* Set up the pipeline */
  /* we add a message handler */
  bus = gst_pipeline_get_bus (GST_PIPELINE (user_data.pipeline));
  bus_watch_id = gst_bus_add_watch (bus, bus_call, &user_data);
  gst_object_unref (bus);

  g_signal_connect (user_data.demuxer, "pad-added", G_CALLBACK (on_pad_added),
      &user_data);
  g_signal_connect (user_data.demuxer, "no-more-pads",
      G_CALLBACK (no_more_pads), NULL);

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
