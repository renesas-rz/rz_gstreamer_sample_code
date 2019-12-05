#!/bin/bash

if [ "$HOSTNAME" = "ek874" ]
then
  echo "RZ/G2E only supports 1280x960 resolution"
  echo "Set 1280x960 resolution as default"
  media-ctl -d /dev/media0 -r
  media-ctl -d /dev/media0 -l "'rcar_csi2 feaa0000.csi2':1 -> 'VIN4 output':0 [1]"
  media-ctl -d /dev/media0 -V "'rcar_csi2 feaa0000.csi2':1 [fmt:UYVY8_2X8/1280x960 field:none]"
  media-ctl -d /dev/media0 -V "'ov5645 3-003c':0 [fmt:UYVY8_2X8/1280x960 field:none]"
  echo "/dev/video0 is configured successfully with resolution 1280x960"
elif [ "$HOSTNAME" = "hihope-rzg2n" ] || [ "$HOSTNAME" = "hihope-rzg2m" ]
then
  if [ $# -ne 2 ]
  then
    echo "Illegal number of parameters"
    echo "Usage: ./setup_MIPI_camera.sh <width> <height>"
    exit 1
  fi

  if ([ "$1" != "1280" ] || [ "$2" != "960" ]) && ([ "$1" != "1920" ] || [ "$2" != "1080" ]) && ([ "$1" != "2592" ] || [ "$2" != "1944" ])
  then
    echo "Error: Unsupported resolution: "$1"x"$2""
    echo "Please try one of the following resolutions:: 1280x960 1920x1080 2592x1944"
    exit 1
  fi
  media-ctl -d /dev/media0 -r
  media-ctl -d /dev/media0 -l "'rcar_csi2 fea80000.csi2':1 -> 'VIN0 output':0 [1]"
  media-ctl -d /dev/media0 -V "'rcar_csi2 fea80000.csi2':1 [fmt:UYVY8_2X8/"$1"x"$2" field:none]"
  media-ctl -d /dev/media0 -V "'ov5645 2-003c':0 [fmt:UYVY8_2X8/"$1"x"$2" field:none]"
  echo "/dev/video0 is configured successfully with resolution "$1"x"$2""
else
  echo "Error: Unsupported board"
fi
