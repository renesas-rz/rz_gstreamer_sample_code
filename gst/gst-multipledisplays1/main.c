#include <stdio.h>
#include <string.h>
#include <gst/gst.h>
#include <stdlib.h>
#include <stdbool.h>
#include <wayland-client.h>

#define INPUT_VIDEO_FILE           "/home/media/videos/vga1.h264"
#define REQUIRED_SCREEN_NUMBERS 2
#define PRIMARY_SCREEN_INDEX 0
#define SECONDARY_SCREEN_INDEX 1

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

bool
get_required_monitors(struct wayland_t *handler, struct screen_t *screens[], int count)
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

int
main (int argc, char *argv[])
{
  struct wayland_t *wayland_handler = NULL;
  struct screen_t *screens[REQUIRED_SCREEN_NUMBERS];
  int screen_numbers = 0;

  GstElement *pipeline, *source, *parser, *decoder, *tee;
  GstElement *queue_1, *video_sink_1;
  GstElement *queue_2, *video_sink_2;

  GstPad *req_pad_1, *sink_pad, *req_pad_2;
  GstBus *bus;
  GstMessage *msg;
  GstPadTemplate *tee_src_pad_template;

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
    g_printerr("Must have at least %d monitors to run the app. Exiting.\n", REQUIRED_SCREEN_NUMBERS);

    destroy_wayland(wayland_handler);
    return -1;
  }

  /* Extract required monitors */
  get_required_monitors(wayland_handler, screens, REQUIRED_SCREEN_NUMBERS);

  const char *input_video_file = INPUT_VIDEO_FILE;

  /* Initialization */
  gst_init (&argc, &argv);

  /* Create GStreamer elements */
  pipeline = gst_pipeline_new ("multiple-display");
  source = gst_element_factory_make ("filesrc", "file-source");
  parser = gst_element_factory_make ("h264parse", "parser");
  decoder = gst_element_factory_make ("omxh264dec", "omxh264-decoder");
  tee = gst_element_factory_make ("tee", "tee-element");

  /* Elements for Video Display 1 */
  queue_1 = gst_element_factory_make ("queue", "queue-1");
  video_sink_1 = gst_element_factory_make ("waylandsink", "video-output-1");

  /* Elements for Video Display 2 */
  queue_2 = gst_element_factory_make ("queue", "queue-2");
  video_sink_2 = gst_element_factory_make ("waylandsink", "video-output-2");

  if (!pipeline || !source || !parser || !decoder || !tee
      || !queue_1 || !video_sink_1
      || !queue_2 || !video_sink_2) {
    g_printerr ("One element could not be created. Exiting.\n");

    destroy_wayland(wayland_handler);
    return -1;
  }

  /* Set up the pipeline */

  /* Set the input file location to the source element */
  g_object_set (G_OBJECT (source), "location", input_video_file, NULL);

  /* Set display position and size for Display 1  */
  g_object_set (G_OBJECT (video_sink_1),
       "position-x", screens[PRIMARY_SCREEN_INDEX]->x,
       "position-y", screens[PRIMARY_SCREEN_INDEX]->y,
       "out-width", screens[PRIMARY_SCREEN_INDEX]->width,
       "out-height", screens[PRIMARY_SCREEN_INDEX]->height, NULL);

  /* Set display position and size for Display 2 */
  g_object_set (G_OBJECT (video_sink_2),
       "position-x", screens[SECONDARY_SCREEN_INDEX]->x,
       "position-y", screens[SECONDARY_SCREEN_INDEX]->y,
       "out-width", screens[SECONDARY_SCREEN_INDEX]->width,
       "out-height", screens[SECONDARY_SCREEN_INDEX]->height, NULL);

  /* Add all elements into the pipeline */
  gst_bin_add_many (GST_BIN (pipeline), source, parser, decoder, tee,
      queue_1, video_sink_1,
      queue_2, video_sink_2, NULL);

  /* Link elements together */
  if (gst_element_link_many (source, parser, decoder, tee, NULL) != TRUE) {
    g_printerr ("Source elements could not be linked.\n");
    gst_object_unref (pipeline);

    destroy_wayland(wayland_handler);
    return -1;
  }
  if (gst_element_link_many (queue_1, video_sink_1,
          NULL) != TRUE) {
    g_printerr ("Elements of Video Display-1 could not be linked.\n");
    gst_object_unref (pipeline);

    destroy_wayland(wayland_handler);
    return -1;
  }
  if (gst_element_link_many (queue_2, video_sink_2,
          NULL) != TRUE) {
    g_printerr ("Elements of Video Display-2 could not be linked.\n");
    gst_object_unref (pipeline);

    destroy_wayland(wayland_handler);
    return -1;
  }

  /* Get a src pad template of Tee */
  tee_src_pad_template =
      gst_element_class_get_pad_template (GST_ELEMENT_GET_CLASS (tee),
      "src_%u");

  /* Get request pad and manually link for Video Display 1 */
  req_pad_1 = gst_element_request_pad (tee, tee_src_pad_template, NULL, NULL);
  sink_pad = gst_element_get_static_pad (queue_1, "sink");
  if (gst_pad_link (req_pad_1, sink_pad) != GST_PAD_LINK_OK) {
    g_print ("tee link failed!\n");
  }
  gst_object_unref (sink_pad);

  /* Get request pad and manually link for Video Display 2 */
  req_pad_2 = gst_element_request_pad (tee, tee_src_pad_template, NULL, NULL);
  sink_pad = gst_element_get_static_pad (queue_2, "sink");
  if (gst_pad_link (req_pad_2, sink_pad) != GST_PAD_LINK_OK) {
    g_print ("tee link failed!\n");
  }
  gst_object_unref (sink_pad);

  /* Set the pipeline to "playing" state */
  g_print ("Now playing:\n");
  if (gst_element_set_state (pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (pipeline);

    destroy_wayland(wayland_handler);
    return -1;
  }

  /* Iterate */
  g_print ("Running...\n");
  bus = gst_element_get_bus (pipeline);
  msg =
      gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
      GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
  gst_object_unref (bus);

  /* Note that because input timeout is GST_CLOCK_TIME_NONE, 
     the gst_bus_timed_pop_filtered() function will block forever until a 
     matching message was posted on the bus (GST_MESSAGE_ERROR or 
     GST_MESSAGE_EOS). */

  /* Playback end. Handle the message */
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

  /* Seek to start and flush all old data */
  gst_element_seek_simple (pipeline, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, 0);

  /* Clean up "wayland_t" structure */
  destroy_wayland(wayland_handler);

  /* Clean up nicely */
  gst_element_release_request_pad (tee, req_pad_1);
  gst_element_release_request_pad (tee, req_pad_2);
  gst_object_unref (req_pad_1);
  gst_object_unref (req_pad_2);

  g_print ("Returned, stopping playback\n");
  gst_element_set_state (pipeline, GST_STATE_CHANGE_READY_TO_NULL);

  g_print ("Deleting pipeline\n");
  gst_object_unref (GST_OBJECT (pipeline));

  return 0;
}
