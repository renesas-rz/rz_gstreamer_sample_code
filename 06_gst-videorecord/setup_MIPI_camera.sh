#!/bin/bash

# Copyright (c) 2023-2025 Renesas Electronics Corporation and/or its affiliates
# SPDX-License-Identifier: MIT-0

if [[ $# -ne 1 ]]; then
  echo "Invalid or missing argument!"
  echo -e "Usage:\n\t./setup_MIPI_camera.sh <resolution>"
  echo -e "\n\tValid resolutions: 640x480, 1280x720 and 1920x1080"
  echo -e "Example:\n\t./setup_MIPI_camera.sh 1920x1080"
  exit -1
fi

if [[ $1 != "640x480" ]] && [[ $1 != "1280x720" ]] && [[ $1 != "1920x1080" ]]; then
  echo "RZV2N only support 3 camera resolutions with MIPI camera"
  echo -e "1. 640x480\n2. 1280x720\n3. 1920x1080"
else
  media=$(ls /sys/class/video4linux/video*/device/ | grep -m1 "media")
  cru=$(cat /sys/class/video4linux/video*/name | grep -m1 "CRU")
  csi2=$(cat /sys/class/video4linux/v4l-subdev*/name | grep -m1 "csi2")
  imx462=$(cat /sys/class/video4linux/v4l-subdev*/name | grep -m1 "imx462")

  media-ctl -d /dev/$media -r
  media-ctl -d /dev/$media -l "'$csi2':1 -> '$cru':0 [1]"
  media-ctl -d /dev/$media -V "'$csi2':1 [fmt:UYVY8_2X8/$1 field:none]"
  media-ctl -d /dev/$media -V "'$imx462':0 [fmt:UYVY8_2X8/$1 field:none]"

  echo "/dev/$media is configured successfully with resolution "$1""
fi