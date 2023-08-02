#!/bin/bash

#*******************************************************************************************
#* FILENAME: detect_camera.sh
#*
#* DESCRIPTION:
#*   Detect 1 camera at a time.
#*   If more than 1 camera are connected, the camera having the smallest order of device names is detected.
#*   Example: 1st camera's device name: /dev/video8.
#*            2nd camera's device name: /dev/video10.
#*            -> The 1st camera is detected.
#*   This script can be used in combination with gst-videorecord application
#*   Example: ./gst-videorecord $( ./detect_camera.sh )
#*
#* AUTHOR: RVC       START DATE: 07/18/2023
#*
#* CHANGES:
#* 
#*******************************************************************************************

# ---------- Define error code global variables ----------

ERR_NO_CAMERA=1
PROG_SUCCESS_CODE=0

# ---------- Define main function variables ----------

PROG_STAT=$PROG_SUCCESS_CODE

# ---------- Main function ----------

for DEV_NAME in $( ls -v /dev/video* )
do
  CHECK_CAMERA=$( v4l2-ctl -d $DEV_NAME --all | grep "Video Capture:" )

  if [ ! -z "$CHECK_CAMERA" ]
  then
    CAMERA_DEV=$DEV_NAME
    echo $CAMERA_DEV
    break
  fi
done

if [ -z $CAMERA_DEV ]
then
  PROG_STAT=$ERR_NO_CAMERA
fi

exit $PROG_STAT
