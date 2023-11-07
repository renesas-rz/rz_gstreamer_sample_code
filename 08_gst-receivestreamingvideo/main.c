/* Copyright (c) 2023 Renesas Electronics Corporation and/or its affiliates */
/* SPDX-License-Identifier: MIT-0 */

#include <stdio.h>
#include <string.h>
#include <gst/gst.h>
#include <stdlib.h>
#include <stdbool.h>
#include <wayland-client.h>
#include <libgen.h>

#define PORT 5000

static GMainLoop *main_loop;

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

typedef struct tag_user_data
{
  GstElement *pipeline;
  GstElement *source;
  GstElement *depayloader;
  GstElement *parser;
  GstElement *decoder;
  GstElement *sink;

  struct screen_t *main_screen;
} UserData;

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

void
signalHandler (int signal)
{
  if (signal == SIGINT) {
    g_main_loop_quit (main_loop);
    g_print ("\n");
  }
}

/* The function bus_call() will be called when a message is received */
static gboolean
bus_call (GstBus * bus, GstMessage * msg, gpointer data)
{
  GMainLoop *loop = (GMainLoop *) data;

  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_ERROR:{
      gchar *debug;
      GError *error;

      gst_message_parse_error (msg, &error, &debug);
      g_free (debug);

      g_printerr ("Error: %s\n", error->message);
      g_error_free (error);

      g_main_loop_quit (loop);
      break;
    }
    default:
      /* Don't care other message */
      break;
  }
  return TRUE;
}

void
set_element_properties (UserData *data)
{
  GstCaps *caps;
 /* For valid RTP packets encapsulated in GstBuffers,
  * we use the caps with mime type application/x-rtp.
  *  Select listening port: 5000 */
  caps = gst_caps_new_empty_simple ("application/x-rtp");
  g_object_set (G_OBJECT (data->source), "port", PORT, "caps", caps, NULL);

  /* Set max-lateness maximum number of nanoseconds that a buffer can be late
   * before it is dropped (-1 unlimited).
   * Generate Quality-of-Service events upstream to FALSE */
  g_object_set (G_OBJECT (data->sink), "max-lateness", -1, "qos", FALSE, NULL);

  /* Set position for displaying (0, 0) */
  g_object_set (G_OBJECT (data->sink), "position-x", data->main_screen->x,
      "position-y", data->main_screen->y, NULL);

  /* unref cap after use */
  gst_caps_unref (caps);

}

int
setup_pipeline (UserData *data)
{
  /* set element properties */
  set_element_properties (data);

  /* Add the elements into the pipeline and link them together */
  /* udp-src -> h264-depay -> h264-parser -> h264-decoder -> video-sink */
  gst_bin_add_many (GST_BIN (data->pipeline), data->source, data->depayloader,
      data->parser, data->decoder, data->sink, NULL);

  if (gst_element_link_many (data->source, data->depayloader, data->parser,
          data->decoder, data->sink, NULL) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    return FALSE;
  }
  return TRUE;
}

int
main (int argc, char *argv[])
{
  struct wayland_t *wayland_handler = NULL;
  UserData user_data;

  GstBus *bus;
  guint bus_watch_id;

  if ((argc != 1)) {
    g_print ("Error: Invalid arguments.\n");
    g_print ("Usage: %s \n", argv[0]);
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

  /* Initialization */
  gst_init (&argc, &argv);
  main_loop = g_main_loop_new (NULL, FALSE);

  /* Create GStreamer elements */
  user_data.pipeline = gst_pipeline_new ("receive-streaming");
  user_data.source = gst_element_factory_make ("udpsrc", "udp-src");
  user_data.depayloader = gst_element_factory_make ("rtph264depay",
                              "h264-depay");
  user_data.parser = gst_element_factory_make ("h264parse", "h264-parser");
  user_data.decoder = gst_element_factory_make ("omxh264dec", "h264-decoder");
  user_data.sink = gst_element_factory_make ("waylandsink", "video-sink");

  if (!user_data.pipeline || !user_data.source || !user_data.depayloader ||
          !user_data.parser || !user_data.decoder || !user_data.sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    destroy_wayland (wayland_handler);
    return -1;
  }

  bus = gst_element_get_bus (user_data.pipeline);
  bus_watch_id = gst_bus_add_watch (bus, bus_call, main_loop);
  gst_object_unref (bus);

  if (setup_pipeline (&user_data) != TRUE) {
    gst_object_unref (user_data.pipeline);
    destroy_wayland (wayland_handler);
    return -1;
  }

  /* Set the pipeline to "playing" state */
  g_print ("Now receiving...\n");
  if (gst_element_set_state (user_data.pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (user_data.pipeline);
    destroy_wayland (wayland_handler);
    return -1;
  }

  /* Handle signals gracefully. */
  signal (SIGINT, signalHandler);

  /* Iterate */
  g_print ("Running...\n");
  g_main_loop_run (main_loop);

  /* Clean up "wayland_t" structure */
  destroy_wayland (wayland_handler);

  /* Clean up nicely */
  g_print ("Returned, stopping receiving...\n");
  g_source_remove (bus_watch_id);
  g_main_loop_unref (main_loop);

  gst_element_set_state (user_data.pipeline, GST_STATE_NULL);

  g_print ("Deleting pipeline...\n");
  gst_object_unref (GST_OBJECT (user_data.pipeline));
  return 0;
}
