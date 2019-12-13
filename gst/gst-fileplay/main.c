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
#define ARG_SCALE            2
#define ARG_COUNT            3
#define INDEX                0
#define AUDIO_SAMPLE_RATE    44100

/* Structure to contain decoder queue information, so we can pass it to callbacks */
typedef struct _CustomData
{
  GstElement *video_queue;
  GstElement *audio_queue;
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
on_pad_added (GstElement * element, GstPad * pad, gpointer data)
{
  CustomData *decoders = (CustomData *) data;
  GstPad *sinkpad;
  GstCaps *new_pad_caps = NULL;
  GstStructure *new_pad_struct = NULL;
  const gchar *new_pad_type = NULL;

  new_pad_caps = gst_pad_query_caps (pad, NULL);
  new_pad_struct = gst_caps_get_structure (new_pad_caps, INDEX);
  new_pad_type = gst_structure_get_name (new_pad_struct);
  /* NOTE:
     gst_pad_query_caps: increase the ref count, need to be unref late.
     gst_caps_get_structure: no need to free or unref, it belongs to the GstCaps.
     gst_structure_get_name: no need to free or unref.
   */

  g_print ("Received new pad '%s' from '%s': %s\n", GST_PAD_NAME (pad),
      GST_ELEMENT_NAME (element), new_pad_type);

  if (g_str_has_prefix (new_pad_type, "audio")) {
    /* In case link this pad with the AAC-decoder sink pad */
    sinkpad = gst_element_get_static_pad (decoders->audio_queue, "sink");
    gst_pad_link (pad, sinkpad);
    gst_object_unref (sinkpad);
    g_print ("Audio pad linked!\n");
  } else if (g_str_has_prefix (new_pad_type, "video")) {
    /* In case link this pad with the omxh264-decoder sink pad */
    sinkpad = gst_element_get_static_pad (decoders->video_queue, "sink");
    gst_pad_link (pad, sinkpad);
    gst_object_unref (sinkpad);
    g_print ("Video pad linked!\n");
  } else {
    g_printerr ("Unexpected pad received.\n");
  }

  if (new_pad_caps != NULL) {
    gst_caps_unref (new_pad_caps);
  }
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
  struct screen_t *main_screen = NULL;

  GstElement *pipeline, *source, *demuxer;
  GstElement *video_queue, *video_parser, *video_decoder, *filter,
             *video_capsfilter, *video_sink;
  GstElement *audio_queue, *audio_decoder, *audio_resample,
             *audio_capsfilter, *audio_sink;
  CustomData decoders_data;
  GstCaps *caps;
  GstBus *bus;
  GstMessage *msg;
  bool fullscreen = false;
  const char* ext;
  char* file_name;

  const gchar *input_file = argv[ARG_INPUT];
  if ((argc > ARG_COUNT) || (argc == 1) || ((argc == ARG_COUNT) && (strcmp (argv[ARG_SCALE], "-s")))) {
    g_print ("Error: Invalid arugments.\n");
    g_print ("Usage: %s <path to MP4 file> [-s]\n", argv[ARG_PROGRAM_NAME]);
    return -1;
  }

  /* Check full-screen option */
  if (argc == ARG_COUNT) {
      fullscreen = true;
  }

  if (!is_file_exist(input_file))
  {
    g_printerr("Cannot find input file: %s. Exiting.\n", input_file);
    return -1;
  }

  file_name = basename ((char*) input_file);
  ext = get_filename_ext (file_name);

  if (strcasecmp ("mp4", ext) != 0) {
    g_print ("Unsupported video type. MP4 format is required\n");
    return -1;
  }

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

  /* Initialisation */
  gst_init (&argc, &argv);

  /* Create gstreamer elements */
  pipeline = gst_pipeline_new ("file-play");
  source = gst_element_factory_make ("filesrc", "file-source");
  demuxer = gst_element_factory_make ("qtdemux", "qt-demuxer");
  /* elements for Video thread */
  video_queue = gst_element_factory_make ("queue", "video-queue");
  video_parser = gst_element_factory_make("h264parse", "h264-parser");
  video_decoder = gst_element_factory_make ("omxh264dec", "omxh264-decoder");
  video_sink = gst_element_factory_make ("waylandsink", "video-output");
  /* elements for Audio thread */
  audio_queue = gst_element_factory_make ("queue", "audio-queue");
  audio_decoder = gst_element_factory_make ("faad", "aac-decoder");
  audio_resample = gst_element_factory_make("audioresample", "audio-resample");
  audio_capsfilter = gst_element_factory_make ("capsfilter", "audio-capsfilter");
  audio_sink = gst_element_factory_make ("alsasink", "audio-output");

  if (!pipeline || !source || !demuxer
      || !video_queue || !video_parser || !video_decoder || !video_sink
      || !audio_queue || !audio_decoder || !audio_resample
      || !audio_capsfilter || !audio_sink) {
    g_printerr ("One element could not be created. Exiting.\n");

    destroy_wayland(wayland_handler);
    return -1;
  }

  /* Set up the pipeline */

  /* Set the input filename to the source element */
  g_object_set (G_OBJECT (source), "location", input_file, NULL);

  /* Set position for displaying (0, 0) */
  g_object_set (G_OBJECT (video_sink), "position-x", main_screen->x, "position-y", main_screen->y, NULL);

  /* Create simple cap which contains audio's sample rate */
  caps = gst_caps_new_simple ("audio/x-raw",
      "rate", G_TYPE_INT, AUDIO_SAMPLE_RATE, NULL);

  /* Add cap to capsfilter element */
  g_object_set (G_OBJECT (audio_capsfilter), "caps", caps, NULL);
  gst_caps_unref (caps);

  /* Add elements into the pipeline:
     file-source | qt-demuxer
     <1> | video-queue | omxh264-decoder | video-output
     <2> | audio-queue | aac-decoder | audio-resample | capsfilter | audio-output
   */
  gst_bin_add_many (GST_BIN (pipeline), source, demuxer,
      video_queue, video_parser, video_decoder, video_sink,
      audio_queue, audio_decoder, audio_resample, audio_capsfilter,
      audio_sink, NULL);

  /* Link the elements together:
     - file-source -> qt-demuxer 
  */
  if (gst_element_link (source, demuxer) != TRUE) {
    g_printerr ("Source and demuxer could not be linked.\n");
    gst_object_unref (pipeline);

    destroy_wayland(wayland_handler);
    return -1;
  }

  /* Link the elements together:
     - audio-queue -> aac-decoder -> audio-resample -> capsfilter -> audio-output
  */
  if (gst_element_link_many (audio_queue, audio_decoder, audio_resample,
          audio_capsfilter, audio_sink, NULL) != TRUE) {
    g_printerr ("Audio elements could not be linked.\n");
    gst_object_unref (pipeline);

    destroy_wayland(wayland_handler);
    return -1;
  }

  if (!fullscreen) {
    /* Link the elements together:
     - video-queue -> h264-parser -> omxh264-decoder -> video-output
    */
    if (gst_element_link_many (video_queue, video_parser, video_decoder, video_sink, NULL) != TRUE) {
      g_printerr ("Video elements could not be linked.\n");
      gst_object_unref (pipeline);

      destroy_wayland(wayland_handler);
      return -1;
    }
  } else {
    filter = gst_element_factory_make ("vspmfilter", "vspm-filter");
    video_capsfilter = gst_element_factory_make ("capsfilter", "video-capsfilter");

    if (!filter || !video_capsfilter) {
      g_printerr ("One element could not be created. Exiting.\n");
      gst_object_unref (pipeline);
      destroy_wayland(wayland_handler);
      return -1;
    }

    /* Set property "dmabuf-use" of vspmfilter to true */
    /* Without it, waylandsink will display broken video */
    g_object_set (G_OBJECT (filter), "dmabuf-use", TRUE, NULL);

    /* Create simple cap which contains video's resolution */
    caps = gst_caps_new_simple ("video/x-raw",
        "width", G_TYPE_INT, main_screen->width,
        "height", G_TYPE_INT, main_screen->height, NULL);

    /* Add cap to capsfilter element */
    g_object_set (G_OBJECT (video_capsfilter), "caps", caps, NULL);
    gst_caps_unref (caps);

    /* Add filter, capsfilter into the pipeline */
    gst_bin_add_many (GST_BIN (pipeline), filter, video_capsfilter, NULL);

    /* Link the elements together:
     - video-queue -> h264-parser -> omxh264-decoder -> video-filter -> capsfilter -> video-output
    */

    if (gst_element_link_many (video_queue, video_parser, video_decoder, filter,
            video_capsfilter, video_sink, NULL) != TRUE) {
      g_printerr ("Video elements could not be linked.\n");
      gst_object_unref (pipeline);

      destroy_wayland(wayland_handler);
      return -1;
    }
  }

  decoders_data.audio_queue = audio_queue;
  decoders_data.video_queue = video_queue;
  g_signal_connect (demuxer, "pad-added", G_CALLBACK (on_pad_added),
      &decoders_data);

  /* note that the demuxer will be linked to the decoder dynamically.
     The reason is that Qt may contain various streams (for example
     video, audio and subtitle). The source pad(s) will be created at run time,
     by the demuxer when it detects the amount and nature of streams.
     Therefore we connect a callback function which will be executed
     when the "pad-added" is emitted. */

  /* set the piline to "playing" state */
  g_print ("Now playing: %s\n", input_file);
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
     the gst_bus_timed_pop_filtered() function will block forever untill a 
     matching message was posted on the bus (GST_MESSAGE_ERROR or 
     GST_MESSAGE_EOS). */

  /* Playback end. Clean up nicely */
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

  /* Clean up "wayland_t" structure */
  destroy_wayland(wayland_handler);

  /* Clean up nicely */
  g_print ("Returned, stopping playback\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_print ("Deleting pipeline\n");
  gst_object_unref (GST_OBJECT (pipeline));

  return 0;
}
