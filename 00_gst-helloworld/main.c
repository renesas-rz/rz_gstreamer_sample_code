/* Copyright (c) 2023 Renesas Electronics Corporation and/or its affiliates */
/* SPDX-License-Identifier: MIT-0 */

#include <gst/gst.h>

int
main (int argc, char *argv[])
{
  GstElement *pipeline;
  GstBus *bus;
  GstMessage *msg;

  /* Initialization */
  gst_init (&argc, &argv);

  pipeline =
      gst_parse_launch ("audiotestsrc num-buffers=100 ! autoaudiosink", NULL);

  /* Set the pipeline to "playing" state */
  g_print ("Now playing: audiotestsrc\n");
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  /* Wait for EOS */
  g_print ("Running...\n");
  bus = gst_element_get_bus (pipeline);
  msg =
      gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
      GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

  /* Note that because input timeout is GST_CLOCK_TIME_NONE,
     the gst_bus_timed_pop_filtered() function will block forever until a
     matching message was posted on the bus (GST_MESSAGE_ERROR or
     GST_MESSAGE_EOS). */
  if (msg != NULL) {
    gst_message_unref (msg);
  }

  /* Out of the main loop, clean up nicely */
  gst_object_unref (bus);
  g_print ("Returned, stopping playback...\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);
  g_print ("Deleting pipeline...\n");
  gst_object_unref (GST_OBJECT (pipeline));
  g_print ("Completed. Goodbye!\n");

  return 0;
}
