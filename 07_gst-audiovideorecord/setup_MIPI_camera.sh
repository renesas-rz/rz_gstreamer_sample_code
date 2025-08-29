#!/bin/bash

# Copyright (c) 2023-2025 Renesas Electronics Corporation and/or its affiliates
# SPDX-License-Identifier: MIT-0

BOARD_NAME=$(uname -n)
CODENAME=$(grep ^VERSION /etc/os-release | sed -n 's/.*(\(.*\)).*/\1/p')

if [[ $# -ne 1 ]]; then
  echo "Invalid or missing argument!"
  echo -e "Usage:\n\t./setup_MIPI_camera.sh <resolution>"
  if [[ "$BOARD_NAME" == *g2l* ]] || [[ "$BOARD_NAME" == *v2l* ]] || [[ "$BOARD_NAME" == *g3e* ]]; then
    echo -e "\n\tValid resolutions: 1280x960 and 1920x1080"
  elif [[ "$BOARD_NAME" == *v2n* || "$BOARD_NAME" == *v2h* ]]; then
    echo -e "\n\tValid resolutions: 640x480, 1280x720 and 1920x1080"
  fi
  echo -e "Example:\n\t./setup_MIPI_camera.sh 1920x1080"
  exit -1
fi

if [[ "$BOARD_NAME" == *g2l* ]] || [[ "$BOARD_NAME" == *v2l* ]] || [[ "$BOARD_NAME" == *g3e* ]]; then
  if [[ $1 != "1280x960" ]] && [[ $1 != "1920x1080" ]]; then
    echo "$BOARD_NAME board only support 2 camera resolutions"
    echo -e "1. 1920x1080\n2. 1280x960"
  else
    if [[ "$BOARD_NAME" == *g2l* && "$CODENAME" == *scarthgap* ]] || [[ "$BOARD_NAME" == *g3e* ]]; then
      csi2=$(cat /sys/class/video4linux/v4l-subdev*/name | grep "csi2" | head -n 1)
      ip=$(cat /sys/class/video4linux/v4l-subdev*/name | grep "cru-ip" | head -n 1)

      media-ctl -d /dev/media0 -r
      media-ctl -d /dev/media0 -l "'${csi2}':1 -> '${ip}':0 [1]"
      media-ctl -d /dev/media0 -V "'${csi2}':1 [fmt:UYVY8_2X8/$1 field:none]"
      media-ctl -d /dev/media0 -V "'ov5645 0-003c':0 [fmt:UYVY8_2X8/$1 field:none]"
      media-ctl -d /dev/media0 -V "'${ip}':0 [fmt:UYVY8_2X8/$1 field:none]"
    elif [[ "$BOARD_NAME" == *g2l* && "$CODENAME" == *dunfell* ]] || [[ "$BOARD_NAME" == *v2l* ]]; then
      media-ctl -d /dev/media0 -r
      media-ctl -d /dev/media0 -V "'ov5645 0-003c':0 [fmt:UYVY8_2X8/$1 field:none]"
      media-ctl -d /dev/media0 -l "'rzg2l_csi2 10830400.csi2':1 -> 'CRU output':0 [1]"
      media-ctl -d /dev/media0 -V "'rzg2l_csi2 10830400.csi2':1 [fmt:UYVY8_2X8/$1 field:none]"
    fi
    echo "/dev/video0 is configured successfully with resolution "$1""
  fi
elif [[ "$BOARD_NAME" == *v2n* || "$BOARD_NAME" == *v2h* ]]; then
  if [[ $1 != "640x480" ]] && [[ $1 != "1280x720" ]] && [[ $1 != "1920x1080" ]]; then
    echo "$BOARD_NAME board only support 3 camera resolutions with MIPI camera"
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
else
    echo "This script is not supported for $BOARD_NAME board on $CODENAME."
fi