#include <gst/gst.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <wayland-client.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#define MIPI_BITRATE_OMXH264ENC 40000000 /* Target bitrate of the encoder for MIPI camera */
#define USB_BITRATE_OMXH264ENC  10485760 /* Target bitrate of the encoder for USB camera */
#define VARIABLE_RATE      1
#define USB_WIDTH_SIZE     640        /* The output data of v4l2src in this application will be */
#define USB_HEIGHT_SIZE    480        /* a raw video with 640x480 size */
#define MIPI_WIDTH_SIZE    1280       /* The output data of v4l2src in this application will be */
#define MIPI_HEIGHT_SIZE   960        /* a raw video with 1280x960 size */
#define ARG_PROGRAM_NAME   0
#define ARG_DEVICE         1
#define ARG_WIDTH          2
#define ARG_HEIGHT         3
#define ARG_COUNT          4

static GstElement *pipeline;

enum camera_type {
  NO_CAMERA,
  MIPI_CAMERA,
  USB_CAMERA
};

/* Supported resolutions of MIPI camera */
const char *mipi_resolutions[] = {
  "1280x960",
  "1920x1080",
  "2592x1944",
  NULL,
};

/* Supported resolutions of USB camera */
const char *usb_resolutions[] = {
  "320x240",
  "640x480",
  "800x600",
  "960x720",
  "1280x720",
  "1280x960",
  NULL,
};

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

/* Check type of camera
 * return NO_CAMERA: Unsupported camera
 * return MIPI_CAMERA: MIPI camera detected
 * return USB_CAMERA: USB camera detected */
enum camera_type
check_camera_type (const char *device)
{
  int fd = -1;
  int ret = -1;
  enum camera_type camera = NO_CAMERA;
  struct v4l2_capability info;

  fd = open (device, O_RDONLY);
  if (fd < 0) {
    g_print ("Cannot open device.\n");
    return NO_CAMERA;
  }

  /* Obtain information about driver */
  ret = ioctl(fd, VIDIOC_QUERYCAP, &info);
  if (ret < 0) {
    g_print ("Invalid V4L2 device.\n");
    close (fd);
    return NO_CAMERA;
  }

  /* Detecting type of camera base on driver*/
  if (strstr ((char*) info.driver, "vin")) {
    g_print ("MIPI camera detected.\n");
    camera = MIPI_CAMERA;
  } else if (strstr ((char*) info.driver, "uvc")) {
    g_print ("USB camera detected.\n");
    camera = USB_CAMERA;
  } else {
    g_print ("Unsupported camera.\n");
    camera = NO_CAMERA;
  }

  close (fd);
  return camera;
}

/* Print supported resolutions in console*/
void
print_supported_resolutions (char *resolution,
    const char* supported_resolutions[]) {
  int index = 0;
  g_print ("%s is unsupported resolution.\n", resolution);
  g_print ("Please try one of the following resolutions:\n");

  /* Print list of supported resolutions */
  while (supported_resolutions[index]) {
    g_print ("%s\n", supported_resolutions[index]);
    index++;
  }
}

/* Check resolution in program argument is supported or not
 * Supported resolutions are defined in
 * usb_resolutions and mipi_resolutions */
bool
check_resolution (char *resolution, const char *supported_resolutions[]) {
  int index = 0;
  bool ret = false;

  while (supported_resolutions[index] != NULL) {
    if (strcasecmp (supported_resolutions[index], resolution) == 0) {
      ret = true;
      break;
    } else {
      index++;
    }
  }

  if (!ret) {
    print_supported_resolutions (resolution, supported_resolutions);
  }

  return ret;
}

/* Check resolution in program argument is supported or not
 * If not, display list of supported resolutions in console
 * else store resolution to width, height */
bool
get_resolution (char *arg_width, char *arg_height, int *width,
    int *height, enum camera_type camera) {
  bool ret = false;
  char resolution[10];

  sprintf (resolution, "%sx%s", arg_width, arg_height);
  if (camera == MIPI_CAMERA) {
    ret = check_resolution (resolution, mipi_resolutions);
  } else {
    ret = check_resolution (resolution, usb_resolutions);
  }

  if (!ret) {
    return ret;
  }

  /* Store resolution to width and height */
  *width = atoi (arg_width);
  *height = atoi (arg_height);

  return ret;
}

void
signalHandler (int signal)
{
  if (signal == SIGINT) {
    gst_element_send_event (pipeline, gst_event_new_eos ());
  }
}

static void
link_to_multiplexer (GstPad * tolink_pad, GstElement * mux)
{
  GstPad *pad;
  gchar *srcname, *sinkname;

  srcname = gst_pad_get_name (tolink_pad);
  pad = gst_element_get_compatible_pad (mux, tolink_pad, NULL);
  gst_pad_link (tolink_pad, pad);
  sinkname = gst_pad_get_name (pad);
  gst_object_unref (GST_OBJECT (pad));

  g_print ("A new pad %s was created and linked to %s\n", sinkname, srcname);
  g_free (sinkname);
  g_free (srcname);
}

int
main (int argc, char *argv[])
{
  struct wayland_t *wayland_handler = NULL;
  bool display_video = true;

  GstElement *source, *camera_capsfilter, *converter, *convert_capsfilter,
      *queue1, *encoder, *parser, *muxer, *filesink;
  GstBus *bus;
  GstMessage *msg;
  GstPad *srcpad;
  GstCaps *camera_caps, *convert_caps;
  enum camera_type camera = NO_CAMERA;
  int width = 0;
  int height = 0;

  const gchar *output_file = "RECORD-camera.mp4";

  /* Get a list of available screen */
  wayland_handler = get_available_screens();

  if (wayland_handler->output == NULL) {
    display_video = false;
  }
  destroy_wayland(wayland_handler);

  if ((argc != 2) && (argc != ARG_COUNT)) {
    g_print ("Error: Invalid arugments.\n");
    g_print ("Usage: %s <camera device> [width] [height]\n", argv[ARG_PROGRAM_NAME]);
    return -1;
  }

  /* Check type of camera and set default resolution */
  camera = check_camera_type (argv[ARG_DEVICE]);
  if (camera == NO_CAMERA) {
    return -1;
  } else if (camera == MIPI_CAMERA) {
    width = MIPI_WIDTH_SIZE;
    height = MIPI_HEIGHT_SIZE;
  } else {
    width = USB_WIDTH_SIZE;
    height = USB_HEIGHT_SIZE;
  }

  /* Parse resolution from program argument */
  if (argc == ARG_COUNT) {
    char hostname[20];
    gethostname (hostname, 20);
    if (!strcmp (hostname, "ek874") && (camera == MIPI_CAMERA)) {
      g_print ("RZ/G2E only supports 1280x960 resolution.\n");
      g_print ("Set 1280x960 resolution as default.\n");
    } else {
      if (!get_resolution (argv[ARG_WIDTH], argv[ARG_HEIGHT],
               &width, &height, camera)) {
        return -1;
      }
    }
  }

  /* Initialization */
  gst_init (&argc, &argv);

  /* Create GStreamer elements */
  pipeline = gst_pipeline_new ("video-record");
  source = gst_element_factory_make ("v4l2src", "camera-source");
  camera_capsfilter = gst_element_factory_make ("capsfilter", "camera_caps");
  convert_capsfilter = gst_element_factory_make ("capsfilter", "convert_caps");
  queue1 = gst_element_factory_make ("queue", "queue1");
  encoder = gst_element_factory_make ("omxh264enc", "video-encoder");
  parser = gst_element_factory_make ("h264parse", "h264-parser");
  muxer = gst_element_factory_make ("qtmux", "mp4-muxer");
  filesink = gst_element_factory_make ("filesink", "file-output");

  if (camera == MIPI_CAMERA) {
    converter = gst_element_factory_make ("vspmfilter", "video-converter");
  } else {
    converter = gst_element_factory_make ("videoconvert", "video-converter");
  }

  if (!pipeline || !source || !camera_capsfilter || !converter
      || !convert_capsfilter || !queue1 || !encoder || !parser
      || !muxer || !filesink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  if (camera == MIPI_CAMERA) {
    /* Set property "dmabuf-use" of vspmfilter to true */
    /* Without it, waylandsink will display broken video */
    g_object_set (G_OBJECT (converter), "dmabuf-use", true, NULL);
    /* Set properties of the encoder element - omxh264enc */
    g_object_set (G_OBJECT (encoder), "target-bitrate", MIPI_BITRATE_OMXH264ENC,
        "control-rate", VARIABLE_RATE, "interval_intraframes", 14,
        "periodicty-idr", 2, "use-dmabuf", true, NULL);

    /* Create camera caps */
    camera_caps =
        gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING, "UYVY",
            "width", G_TYPE_INT, width, "height", G_TYPE_INT, height, NULL);
  } else {
    /* Set properties of the encoder element - omxh264enc */
    g_object_set (G_OBJECT (encoder), "target-bitrate", USB_BITRATE_OMXH264ENC,
        "control-rate", VARIABLE_RATE, NULL);

    /* Create camera caps */
    camera_caps =
        gst_caps_new_simple ("video/x-raw", "width", G_TYPE_INT, width,
            "height", G_TYPE_INT, height, NULL);
  }

  /* Set input video device file of the source element - v4l2src */
  g_object_set (G_OBJECT (source), "device", argv[ARG_DEVICE], NULL);

  /* Set output file location of the filesink element - filesink */
  g_object_set (G_OBJECT (filesink), "location", output_file, NULL);

  /* create convert caps */
  convert_caps =
      gst_caps_new_simple ("video/x-raw", "format", G_TYPE_STRING, "NV12",
      NULL);

  /* set caps property for capsfilters */
  g_object_set (G_OBJECT (camera_capsfilter), "caps", camera_caps, NULL);
  g_object_set (G_OBJECT (convert_capsfilter), "caps", convert_caps, NULL);

  /* unref caps after usage */
  gst_caps_unref (camera_caps);
  gst_caps_unref (convert_caps);

  /* Add elements into the pipeline */
  gst_bin_add_many (GST_BIN (pipeline), source, camera_capsfilter, converter,
      convert_capsfilter, queue1, encoder, parser, muxer, filesink, NULL);

  if (!display_video) {
    /* Link the elements together */
    if (gst_element_link_many (source, camera_capsfilter, converter,
            convert_capsfilter, queue1, encoder, parser, NULL) != TRUE) {
      g_printerr ("Elements could not be linked.\n");
      gst_object_unref (pipeline);
      return -1;
    }
  } else {
    GstElement *tee, *queue2, *waylandsink;

    /* Create tee and waylandsink */
    tee = gst_element_factory_make ("tee", "tee");
    queue2 = gst_element_factory_make ("queue", "queue2");
    waylandsink = gst_element_factory_make ("waylandsink", "video-output");

    if (!tee || !queue2 || !waylandsink) {
      g_printerr ("One element could not be created. Exiting.\n");
      return -1;
    }

    /* Set position of video - waylandsink */
    g_object_set (G_OBJECT (waylandsink), "position-x", 0, "position-y", 0, NULL);

    /* Add tee and waylandsink into the pipeline */
    gst_bin_add_many (GST_BIN (pipeline), tee, queue2, waylandsink, NULL);

    /* Link the elements together */
    if (gst_element_link_many (source, camera_capsfilter, converter,
            convert_capsfilter, tee, NULL) != TRUE) {
      g_printerr ("Elements could not be linked.\n");
      gst_object_unref (pipeline);
      return -1;
    }
    if (gst_element_link_many (tee, queue1, encoder, parser, NULL) != TRUE) {
      g_printerr ("Elements could not be linked.\n");
      gst_object_unref (pipeline);
      return -1;
    }
    if (gst_element_link_many (tee, queue2, waylandsink, NULL) != TRUE) {
      g_printerr ("Elements could not be linked.\n");
      gst_object_unref (pipeline);
      return -1;
    }
  }

  /* link qtmuxer->filesink */
  if (gst_element_link (muxer, filesink) != TRUE) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  /* link srcpad of h264parse to request pad of qtmuxer */
  srcpad = gst_element_get_static_pad (parser, "src");
  link_to_multiplexer (srcpad, muxer);
  gst_object_unref (srcpad);

  /* Set the pipeline to "playing" state */
  g_print ("Now recording, press [Ctrl] + [C] to stop...\n");
  if (gst_element_set_state (pipeline,
          GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (pipeline);
    return -1;
  }

  /* Handle signals gracefully. */
  signal (SIGINT, signalHandler);

  /* Wait until error or EOS */
  bus = gst_element_get_bus (pipeline);
  msg =
      gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
      GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
  gst_object_unref (bus);

  /* Note that because input timeout is GST_CLOCK_TIME_NONE,
     the gst_bus_timed_pop_filtered() function will block forever until a
     matching message was posted on the bus (GST_MESSAGE_ERROR or
     GST_MESSAGE_EOS). */

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

  /* Clean up nicely */
  g_print ("Returned, stopping recording...\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_print ("Deleting pipeline...\n");
  gst_object_unref (GST_OBJECT (pipeline));
  g_print ("Succeeded. Please check output file: %s\n", output_file);
  return 0;
}
