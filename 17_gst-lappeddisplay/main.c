#include <stdio.h>
#include <string.h>
#include <gst/gst.h>
#include <stdlib.h>
#include <stdbool.h>
#include <wayland-client.h>

#define INPUT_VIDEO_FILE_1         "/home/media/videos/vga1.h264"
#define INPUT_VIDEO_FILE_2         "/home/media/videos/vga2.h264"
#define INPUT_VIDEO_FILE_3         "/home/media/videos/vga3.h264"

typedef struct _CustomData
{
  GMainLoop *loop;
  int loop_reference;
  GMutex mutex;
} CustomData;

/* These structs contain information needed to get a list of available screens */
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
  /* Don't bother waiting for this; there's no good reason a
   * compositor will wait more than one roundtrip before sending
   * these initial events. */
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
      handler->output = wl_registry_bind(handler->registry, id, &wl_output_interface, MIN(version, 2));
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

  /* Obtain wl_registry from Wayland compositor to access public object "wl_output" */
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
create_video_pipeline (GstElement ** p_video_pipeline, const gchar * input_file,
    struct screen_t * screen, CustomData * data)
{
  GstBus *bus;
  guint video_bus_watch_id;
  GstElement *video_source, *video_parser, *video_decoder,
             *filter, *capsfilter, *video_sink;
  GstCaps *caps;

  /* Create GStreamer elements for video play */
  *p_video_pipeline = gst_pipeline_new (NULL);
  video_source = gst_element_factory_make ("filesrc", NULL);
  video_parser = gst_element_factory_make ("h264parse", NULL);
  video_decoder = gst_element_factory_make ("omxh264dec", NULL);
  filter = gst_element_factory_make ("vspmfilter", NULL);
  capsfilter = gst_element_factory_make ("capsfilter", NULL);
  video_sink = gst_element_factory_make ("waylandsink", NULL);

  if (!*p_video_pipeline || !video_source || !video_parser || !video_decoder
      || !filter || !capsfilter || !video_sink) {
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
      video_decoder, filter, capsfilter, video_sink, NULL);

  /* Set up for the video pipeline */
  /* Set the input file location of the file source element */
  g_object_set (G_OBJECT (video_source), "location", input_file, NULL);

  g_object_set (G_OBJECT (video_sink), "position-x", screen->x, "position-y",
      screen->y, NULL);

  /* Set property "dmabuf-use" of vspmfilter to true */
  /* Without it, waylandsink will display broken video */
  g_object_set (G_OBJECT (filter), "dmabuf-use", TRUE, NULL);

  /* Create simple cap which contains video's resolution */
  caps = gst_caps_new_simple ("video/x-raw",
      "width", G_TYPE_INT, screen->width,
      "height", G_TYPE_INT, screen->height, NULL);

  /* Add cap to capsfilter element */
  g_object_set (G_OBJECT (capsfilter), "caps", caps, NULL);
  gst_caps_unref (caps);

  /* Link the elements together */
  /* file-source -> h264-parser -> h264-decoder -> video-output */
  if (!gst_element_link_many (video_source, video_parser, video_decoder,
          filter, capsfilter, video_sink, NULL)) {
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
  } else {
    /* wait until the changing is complete */
    gst_element_get_state (pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
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

int
main (int argc, char *argv[])
{
  struct wayland_t *wayland_handler = NULL;
  struct screen_t *main_screen = NULL;
  struct screen_t temp;

  CustomData shared_data;
  GstElement *video_pipeline_1;
  GstElement *video_pipeline_2;
  GstElement *video_pipeline_3;

  guint video_bus_watch_id_1;
  guint video_bus_watch_id_2;
  guint video_bus_watch_id_3;

  const gchar *input_video_file_1 = INPUT_VIDEO_FILE_1;
  const gchar *input_video_file_2 = INPUT_VIDEO_FILE_2;
  const gchar *input_video_file_3 = INPUT_VIDEO_FILE_3;

  /* Get a list of available screen */
  wayland_handler = get_available_screens();

  /* Get main screen */
  main_screen = get_main_screen(wayland_handler);
  if (main_screen == NULL)
  {
    g_printerr("Cannot find any available screens. Exiting.\n");
    destroy_wayland(wayland_handler);
    return -1;
  }

  /* Check input file */
  if (!is_file_exist(input_video_file_1) || !is_file_exist(input_video_file_2) ||
      !is_file_exist(input_video_file_3))
  {
    g_printerr("Make sure the following (H264-encoded) files exist:\n");
    g_printerr("  %s\n", input_video_file_1);
    g_printerr("  %s\n", input_video_file_2);
    g_printerr("  %s\n", input_video_file_3);

    destroy_wayland(wayland_handler);
    return -1;
  }

  /* Initialization */
  gst_init (&argc, &argv);
  shared_data.loop = g_main_loop_new (NULL, FALSE);
  shared_data.loop_reference = 0; 	/* Counter */
  g_mutex_init (&shared_data.mutex);

  /* The video will be displayed with VGA size */
  temp.width = 640;
  temp.height = 480;

  /* Create first pipeline */
  temp.x = 0;
  temp.y = 0;
  video_bus_watch_id_1 =
      create_video_pipeline (&video_pipeline_1, input_video_file_1,
      &temp, &shared_data);

  if (video_bus_watch_id_1 == 0)
  {
    destroy_wayland(wayland_handler);
    return -1;
  }

  /* Create second pipeline */
  temp.x = main_screen->width / 4;
  temp.y = main_screen->height / 4;
  video_bus_watch_id_2 =
      create_video_pipeline (&video_pipeline_2, input_video_file_2,
      &temp, &shared_data);

  if (video_bus_watch_id_2 == 0)
  {
    destroy_wayland(wayland_handler);
    return -1;
  }

  /* Create third pipeline */
  temp.x = main_screen->width / 2;
  temp.y = main_screen->height / 2;
  video_bus_watch_id_3 =
      create_video_pipeline (&video_pipeline_3, input_video_file_3,
      &temp, &shared_data);

  if (video_bus_watch_id_3 == 0)
  {
    destroy_wayland(wayland_handler);
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

    destroy_wayland(wayland_handler);
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

    destroy_wayland(wayland_handler);
    return -1;
  }

  /* Set the video pipeline 3 to "playing" state */
  if (play_pipeline (video_pipeline_3, &shared_data) == true)
  {
    g_print ("Now playing video file: %s\n", input_video_file_3);
  }
  else
  {
    g_printerr("Unable to play video file: %s\n", input_video_file_3);

    destroy_wayland(wayland_handler);
    return -1;
  }

  /* Iterate */
  g_print ("Running...\n");
  g_main_loop_run (shared_data.loop);

  /* Clean up "wayland_t" structure */
  destroy_wayland(wayland_handler);

  /* Out of loop. Clean up nicely */
  g_source_remove (video_bus_watch_id_1);
  g_source_remove (video_bus_watch_id_2);
  g_source_remove (video_bus_watch_id_3);
  g_main_loop_unref (shared_data.loop);
  g_mutex_clear (&shared_data.mutex);
  g_print ("Program end!\n");

  return 0;
}
