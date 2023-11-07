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

#define ARG_PROGRAM_NAME     0
#define ARG_INPUT            1
#define ARG_COUNT            2
#define REQUIRED_SCREEN_NUMBERS 1
#define PRIMARY_SCREEN_INDEX 0
#define SECONDARY_SCREEN_INDEX 0
#define PRIMARY_POS_OFFSET   0
#define SECONDARY_POS_OFFSET   300

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
  GstElement *pipeline;
  GstElement *source;
  GstElement *parser;
  GstElement *decoder;
  GstElement *tee;
  GstElement *queue_1;
  GstElement *video_sink_1;
  GstElement *queue_2;
  GstElement *video_sink_2;

  GstPad *req_pad_1;
  GstPad *req_pad_2;

  const char *input_video_file;
  struct screen_t *screens[REQUIRED_SCREEN_NUMBERS];
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

void
set_element_properties (UserData *data)
{
  /* Set the input file location to the source element */
  g_object_set (G_OBJECT (data->source),
      "location", data->input_video_file, NULL);

  /* Set display position and size for Display 1  */
  g_object_set (G_OBJECT (data->video_sink_1),
       "position-x",
       data->screens[PRIMARY_SCREEN_INDEX]->x + PRIMARY_POS_OFFSET,
       "position-y",
       data->screens[PRIMARY_SCREEN_INDEX]->y + PRIMARY_POS_OFFSET, NULL);

  /* Set display position and size for Display 2 */
  g_object_set (G_OBJECT (data->video_sink_2),
       "position-x",
       data->screens[SECONDARY_SCREEN_INDEX]->x + SECONDARY_POS_OFFSET,
       "position-y",
       data->screens[SECONDARY_SCREEN_INDEX]->y + SECONDARY_POS_OFFSET, NULL);
}

int
setup_pipeline (UserData *data)
{
  GstPadTemplate *tee_src_pad_template;
  GstPad *sink_pad;

  /* set element properties */
  set_element_properties (data);

  /* Add the elements into the pipeline and link them together */
  /* file-source -> h264-parser -> omxh264-decoder -> tee -> <1>, <2>
   * <1> video-queue1 -> video-output1
   * <2> video-queue2 -> video-output2 */
  gst_bin_add_many (GST_BIN (data->pipeline),
      data->source, data->parser, data->decoder, data->tee, data->queue_1,
      data->video_sink_1, data->queue_2, data->video_sink_2, NULL);

  /* source -> h264-parser -> omxh264-decoder -> tee */
  if (gst_element_link_many (data->source, data->parser, data->decoder,
          data->tee, NULL) != TRUE) {
    g_printerr ("Source elements could not be linked.\n");
    return FALSE;
  }
  /* <1> */
  if (gst_element_link_many (data->queue_1, data->video_sink_1,
          NULL) != TRUE) {
    g_printerr ("Elements of Video Display-1 could not be linked.\n");
    return FALSE;
  }
  /* <2> */
  if (gst_element_link_many (data->queue_2, data->video_sink_2,
          NULL) != TRUE) {
    g_printerr ("Elements of Video Display-2 could not be linked.\n");
    return FALSE;
  }

  /* Get a src pad template of Tee */
  tee_src_pad_template =
      gst_element_class_get_pad_template (GST_ELEMENT_GET_CLASS (data->tee),
      "src_%u");

  /* Get request pad and manually link for Video Display 1 */
  data->req_pad_1 = gst_element_request_pad (data->tee, tee_src_pad_template,
                        NULL, NULL);
  sink_pad = gst_element_get_static_pad (data->queue_1, "sink");
  if (gst_pad_link (data->req_pad_1, sink_pad) != GST_PAD_LINK_OK) {
    g_print ("tee link failed!\n");
  }
  gst_object_unref (sink_pad);

  /* Get request pad and manually link for Video Display 2 */
  data->req_pad_2 = gst_element_request_pad (data->tee, tee_src_pad_template,
                    NULL, NULL);
  sink_pad = gst_element_get_static_pad (data->queue_2, "sink");
  if (gst_pad_link (data->req_pad_2, sink_pad) != GST_PAD_LINK_OK) {
    g_print ("tee link failed!\n");
  }
  gst_object_unref (sink_pad);

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
  struct wayland_t *wayland_handler = NULL;
  UserData user_data;
  int screen_numbers = 0;
  const char* ext;
  char* file_name;

  GstBus *bus;
  GstMessage *msg;

  user_data.input_video_file = argv[ARG_INPUT];

  if (argc != ARG_COUNT) {
    g_print ("Error: Invalid arugments.\n");
    g_print ("Usage: %s <path to H264 file> \n", argv[ARG_PROGRAM_NAME]);
    return -1;
  }

  file_name = basename ((char*) user_data.input_video_file);
  ext = get_filename_ext (file_name);

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
  if (!is_file_exist(user_data.input_video_file))
  {
    g_printerr("Cannot find input file: %s. Exiting.\n",
        user_data.input_video_file);

    destroy_wayland (wayland_handler);
    return -1;
  }

  /* Extract required monitors */
  get_required_monitors(wayland_handler, user_data.screens,
      REQUIRED_SCREEN_NUMBERS);

  /* Initialization */
  gst_init (&argc, &argv);

  /* Check the extension and create parser, decoder */
  if ((strcasecmp ("h264", ext) == 0) || (strcasecmp ("264", ext) == 0)) {
    user_data.parser = gst_element_factory_make ("h264parse", "h264-parser");
    user_data.decoder = gst_element_factory_make ("omxh264dec",
                            "h264-decoder");
  } else {
    g_print ("Unsupported video type. H264 format is required.\n");
    destroy_wayland (wayland_handler);
    return -1;
  }

  /* Create GStreamer elements */
  user_data.pipeline = gst_pipeline_new ("multiple-display");
  user_data.source = gst_element_factory_make ("filesrc", "file-source");
  user_data.tee = gst_element_factory_make ("tee", "tee-element");

  /* Elements for Video Display 1 */
  user_data.queue_1 = gst_element_factory_make ("queue", "queue-1");
  user_data.video_sink_1 = gst_element_factory_make ("waylandsink",
                               "video-output-1");

  /* Elements for Video Display 2 */
  user_data.queue_2 = gst_element_factory_make ("queue", "queue-2");
  user_data.video_sink_2 = gst_element_factory_make ("waylandsink",
                               "video-output-2");

  if (!user_data.pipeline || !user_data.source || !user_data.parser ||
          !user_data.decoder || !user_data.tee || !user_data.queue_1 ||
          !user_data.video_sink_1 || !user_data.queue_2 ||
          !user_data.video_sink_2) {
    g_printerr ("One element could not be created. Exiting.\n");

    destroy_wayland (wayland_handler);
    return -1;
  }

  if (setup_pipeline (&user_data) != TRUE) {
    gst_object_unref (user_data.pipeline);

    destroy_wayland (wayland_handler);
    return -1;
  }

  /* Set the pipeline to "playing" state */
  g_print ("Now playing:\n");
  if (gst_element_set_state (user_data.pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (user_data.pipeline);

    destroy_wayland (wayland_handler);
    return -1;
  }

  /* Iterate */
  g_print ("Running...\n");
  bus = gst_element_get_bus (user_data.pipeline);
  msg =
      gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
      GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
  gst_object_unref (bus);

  /* Note that because input timeout is GST_CLOCK_TIME_NONE,
     the gst_bus_timed_pop_filtered() function will block forever until a
     matching message was posted on the bus (GST_MESSAGE_ERROR or
     GST_MESSAGE_EOS). */

  /* Seek to start and flush all old data */
  gst_element_seek_simple (user_data.pipeline, GST_FORMAT_TIME,
      GST_SEEK_FLAG_FLUSH, 0);

  /* Clean up "wayland_t" structure */
  destroy_wayland (wayland_handler);

  if (msg != NULL) {
    parse_message (msg);
    gst_message_unref (msg);
  }

  g_print ("Returned, stopping playback\n");
  gst_element_set_state (user_data.pipeline, GST_STATE_NULL);
  g_print ("Deleting pipeline\n");
  gst_object_unref (GST_OBJECT (user_data.pipeline));

  return 0;
}

