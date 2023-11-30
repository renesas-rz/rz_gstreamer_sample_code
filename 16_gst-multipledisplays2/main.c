/* Copyright (c) 2023 Renesas Electronics Corporation and/or its affiliates */
/* SPDX-License-Identifier: MIT-0 */

#include <stdio.h>
#include <string.h>
#include <gst/gst.h>
#include <stdlib.h>
#include <stdbool.h>
#include <wayland-client.h>
#include <strings.h>
#include <libgen.h>

#define ARG_PROGRAM_NAME      0
#define ARG_INPUT1            1
#define ARG_INPUT2            2
#define ARG_COUNT             3
#define REQUIRED_SCREEN_NUMBERS 1
#define PRIMARY_SCREEN_INDEX 0
#define SECONDARY_SCREEN_INDEX 0
#define PRIMARY_POS_OFFSET 0
#define SECONDARY_POS_OFFSET 300

typedef struct _CustomData
{
  GMainLoop *loop;
  int loop_reference;
  GMutex mutex;
  const char *video_ext;
} CustomData;

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

bool
get_required_monitors(struct wayland_t *handler, struct screen_t *screens[],
    int count)
{
  bool result = false;
  struct screen_t *screen = NULL;
  int index = 0;

  if ((handler != NULL) && (screens != NULL) && (count > 0) &&
      (wl_list_length(&(handler->screens)) >= count))
  {
    wl_list_for_each(screen, &(handler->screens), link)
    {
      screens[index] = screen;

      index++;
      if (index == count)
      {
        result = true;
        break;
      }
    }
  }

  return result;
}

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

guint
create_video_pipeline (GstElement ** p_video_pipeline,
    const gchar * input_file, struct screen_t * screen, CustomData * data)
{
  GstBus *bus;
  guint video_bus_watch_id;
  GstElement *video_source, *video_parser, *video_decoder, *video_sink;

  /* Create GStreamer elements for video play */
  *p_video_pipeline = gst_pipeline_new (NULL);
  video_source = gst_element_factory_make ("filesrc", NULL);
  video_sink = gst_element_factory_make ("waylandsink", NULL);

  if ((strcasecmp ("h264", data->video_ext) == 0) || (strcasecmp ("264",
          data->video_ext) == 0)) {
    video_parser = gst_element_factory_make ("h264parse", "h264-parser");
    video_decoder = gst_element_factory_make ("omxh264dec", "h264-decoder");
  }

  if (!*p_video_pipeline || !video_source || !video_parser || !video_decoder ||
          !video_sink) {
    g_printerr ("One video element could not be created. Exiting.\n");
    return 0;
  }

  /* Adding a message handler for video pipeline */
  bus = gst_pipeline_get_bus (GST_PIPELINE (*p_video_pipeline));
  video_bus_watch_id = gst_bus_add_watch (bus, bus_call, data);
  gst_object_unref (bus);

  /* Add all elements into the video pipeline */
  /* file-source | parser | decoder | filter | capsfilter | video-output */
  gst_bin_add_many (GST_BIN (*p_video_pipeline), video_source, video_parser,
      video_decoder, video_sink, NULL);

  /* Set up for the video pipeline */
  /* Set the input file location of the file source element */
  g_object_set (G_OBJECT (video_source), "location", input_file, NULL);
  g_object_set (G_OBJECT (video_sink),
      "position-x", screen->x, "position-y", screen->y, NULL);

  /* Link the elements together */
  /* file-source -> parser -> decoder -> video-output */
  if (!gst_element_link_many (video_source, video_parser, video_decoder,
          video_sink, NULL)) {
    g_printerr ("Video elements could not be linked.\n");
    gst_object_unref (*p_video_pipeline);
    return 0;
  }

  return video_bus_watch_id;
}

bool
play_pipeline (GstElement * pipeline, CustomData * p_shared_data)
{
  bool result = true;

  g_mutex_lock (&p_shared_data->mutex);
  ++(p_shared_data->loop_reference);
  if (gst_element_set_state (pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    --(p_shared_data->loop_reference);
    gst_object_unref (pipeline);

    result = false;
  }
  g_mutex_unlock (&p_shared_data->mutex);

  return result;
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

int
main (int argc, char *argv[])
{
  struct wayland_t *wayland_handler = NULL;
  struct screen_t *screens[REQUIRED_SCREEN_NUMBERS];
  int screen_numbers = 0;

  CustomData shared_data;
  GstElement *video_pipeline_1;
  GstElement *video_pipeline_2;
  guint video_bus_watch_id_1;
  guint video_bus_watch_id_2;

  const gchar *input_video_file_1 = argv[ARG_INPUT1];
  const gchar *input_video_file_2 = argv[ARG_INPUT2];
  const char* video1_ext;
  const char* video2_ext;
  char* file_name_1;
  char* file_name_2;

  if (argc != ARG_COUNT) {
    g_print ("Error: Invalid arugments.\n");
    g_print ("Usage: %s <path to the first H264 file> <path to the second" \
        "H264 file> \n", argv[ARG_PROGRAM_NAME]);
    return -1;
  }

  file_name_1 = basename ((char*) input_video_file_1);
  file_name_2 = basename ((char*) input_video_file_2);
  video1_ext = get_filename_ext (file_name_1);
  video2_ext = get_filename_ext (file_name_2);

  if ((strcasecmp ("h264", video1_ext) != 0) && (strcasecmp ("264",
           video1_ext) != 0)) {
    g_print ("Unsupported video type. H264 format is required\n");
    return -1;
  }

  if ((strcasecmp ("h264", video2_ext) != 0) && (strcasecmp ("264",
           video2_ext) != 0)) {
    g_print ("Unsupported video type. H264 format is required\n");
    return -1;
  }

  /* Get a list of available screen */
  wayland_handler = get_available_screens();
  if (wayland_handler == NULL)
  {
    g_printerr("Cannot detect monitors. Exiting.\n");
    return -1;
  }

  screen_numbers = wl_list_length(&(wayland_handler->screens));
  if (screen_numbers < REQUIRED_SCREEN_NUMBERS)
  {
    g_printerr("Detected %d monitors.\n", screen_numbers);
    g_printerr("Must have at least %d monitors to run the app. Exiting.\n",
        REQUIRED_SCREEN_NUMBERS);

    destroy_wayland (wayland_handler);
    return -1;
  }

  /* Check input file */
  if (!is_file_exist(input_video_file_1) || !is_file_exist(input_video_file_2))
  {
    g_printerr("Make sure the following (H264-encoded) files exist:\n");
    g_printerr("  %s\n", input_video_file_1);
    g_printerr("  %s\n", input_video_file_2);

    destroy_wayland (wayland_handler);
    return -1;
  }

  /* Extract required monitors */
  get_required_monitors(wayland_handler, screens, REQUIRED_SCREEN_NUMBERS);

  /* Initialization */
  gst_init (&argc, &argv);
  shared_data.loop = g_main_loop_new (NULL, FALSE);
  shared_data.loop_reference = 0; 	/* counter */
  shared_data.video_ext = video1_ext;
  g_mutex_init (&shared_data.mutex);

  screens[PRIMARY_SCREEN_INDEX]->x += PRIMARY_POS_OFFSET;
  screens[PRIMARY_SCREEN_INDEX]->y += PRIMARY_POS_OFFSET;
  video_bus_watch_id_1 =
      create_video_pipeline (&video_pipeline_1, input_video_file_1,
      screens[PRIMARY_SCREEN_INDEX], &shared_data);

  if (video_bus_watch_id_1 == 0)
  {
    destroy_wayland (wayland_handler);
    return -1;
  }

  shared_data.video_ext = video2_ext;

  screens[SECONDARY_SCREEN_INDEX]->x += SECONDARY_POS_OFFSET;
  screens[SECONDARY_SCREEN_INDEX]->y += SECONDARY_POS_OFFSET;
  video_bus_watch_id_2 =
      create_video_pipeline (&video_pipeline_2, input_video_file_2,
      screens[SECONDARY_SCREEN_INDEX], &shared_data);

  if (video_bus_watch_id_2 == 0)
  {
    destroy_wayland (wayland_handler);
    return -1;
  }

  /* Set the video pipeline 1 to "playing" state */
  if (play_pipeline (video_pipeline_1, &shared_data) == true)
  {
    g_print ("Now playing video file: %s\n", input_video_file_1);
  }
  else
  {
    g_printerr("Unable to play video file: %s\n", input_video_file_1);

    destroy_wayland (wayland_handler);
    return -1;
  }

  /* Set the video pipeline 2 to "playing" state */
  if (play_pipeline (video_pipeline_2, &shared_data) == true)
  {
    g_print ("Now playing video file: %s\n", input_video_file_2);
  }
  else
  {
    g_printerr("Unable to play video file: %s\n", input_video_file_2);

    destroy_wayland (wayland_handler);
    return -1;
  }

  /* Iterate */
  g_print ("Running...\n");
  g_main_loop_run (shared_data.loop);

  /* Clean up "wayland_t" structure */
  destroy_wayland (wayland_handler);

  /* Out of loop. Clean up nicely */
  g_source_remove (video_bus_watch_id_1);
  g_source_remove (video_bus_watch_id_2);
  g_main_loop_unref (shared_data.loop);
  g_mutex_clear (&shared_data.mutex);
  g_print ("Program end!\n");

  return 0;
}