#!/bin/bash

# Copyright (c) 2023-2025 Renesas Electronics Corporation and/or its affiliates
# SPDX-License-Identifier: MIT-0

[ -z $1 ] && echo -e "Please input resolution !\nTry: ./setup_MIPI_camera.sh 1920x1080 " && exit -1
if [[ $1 != "1280x960" ]] && [[ $1 != "1920x1080" ]]; then
  echo "RZG2L only support 2 camera resolutions"
  echo -e "1. 1920x1080\n2. 1280x960"
else
  csi2=$(cat /sys/class/video4linux/v4l-subdev*/name | grep "csi2" | head -n 1)
  ip=$(cat /sys/class/video4linux/v4l-subdev*/name | grep "cru-ip" | head -n 1)

  media-ctl -d /dev/media0 -r
  media-ctl -d /dev/media0 -l "'${csi2}':1 -> '${ip}':0 [1]"
  media-ctl -d /dev/media0 -V "'${csi2}':1 [fmt:UYVY8_2X8/$1 field:none]"
  media-ctl -d /dev/media0 -V "'ov5645 0-003c':0 [fmt:UYVY8_2X8/$1 field:none]"
  media-ctl -d /dev/media0 -V "'${ip}':0 [fmt:UYVY8_2X8/$1 field:none]"

  echo "/dev/video0 is configured successfully with resolution "$1""
fi