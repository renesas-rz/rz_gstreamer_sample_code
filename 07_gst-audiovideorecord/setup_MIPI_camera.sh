#!/bin/bash

# Copyright (c) 2023-2025 Renesas Electronics Corporation and/or its affiliates
# SPDX-License-Identifier: MIT-0

board=$(uname -n)
case "$board" in
  *rzg2l*|*rzv2l*|*rzg3e*) valid_resolutions=("1280x960" "1920x1080");;
  *rzv2n*|*rzv2h*)         valid_resolutions=("640x480" "1280x720" "1920x1080");;
  *) echo "This script it not supported on ${board}"; exit 1;;
esac

if [ $# -ne 1 ]; then
  echo "Invalid or missing argument!"
  echo "Please try: ${0} -h (or --help) for more details!"
  exit 1
fi

if [[ "$1" == "-h" ]] || [[ "$1" == "--help" ]]; then
  echo -e "Usage: ${0} <resolution>\n"
  echo -e "Example: ${0} 1920x1080\n"
  echo -e "Valid resolutions for ${board}: ${valid_resolutions[@]}"
  exit 0
fi

if [[ ! "${valid_resolutions[@]}" =~ "$1" ]]; then
  echo "Invalid or unsupport resolution: ${1}"
  echo "Please try: ${0} -h (or --help) for more details!"
  exit 1
fi

video4linux="/sys/class/video4linux"
if ! grep -q 'CRU' ${video4linux}/video*/name &> /dev/null; then
  echo "No CRU video device found!"
  exit 1
fi

media=$(ls ${video4linux}/video*/device/ | grep -m1 "media")
csi2=$(cat ${video4linux}/v4l-subdev*/name | grep -m1 "csi2")
sensor="$(grep -h -m1 -E 'imx462|ov5645' ${video4linux}/v4l-subdev*/name)"
ip=$(cat ${video4linux}/v4l-subdev*/name | grep "cru-ip" | head -n 1)
if [ $(printf '%s\n' "$sensor" | sed '/^$/d' | wc -l) -gt 1 ]; then
  echo "Only one MIPI camera can be used at once"
  exit 1
fi

if [ -z "$ip" ]; then
    media-ctl -d /dev/${media} -r
    media-ctl -d /dev/${media} -l "'${csi2}':1 -> 'CRU output':0 [1]"
    media-ctl -d /dev/${media} -V "'${csi2}':1 [fmt:UYVY8_2X8/${1} field:none]"
    media-ctl -d /dev/${media} -V "'${sensor}':0 [fmt:UYVY8_2X8/${1} field:none]"
else
    media-ctl -d /dev/${media} -r
    media-ctl -d /dev/${media} -l "'${csi2}':1 -> '${ip}':0 [1]"
    media-ctl -d /dev/${media} -V "'${csi2}':1 [fmt:UYVY8_2X8/${1} field:none]"
    media-ctl -d /dev/${media} -V "'${sensor}':0 [fmt:UYVY8_2X8/${1} field:none]"
    media-ctl -d /dev/${media} -V "'${ip}':0 [fmt:UYVY8_2X8/${1} field:none]"
fi

echo "/dev/${media} is configured successfully with resolution ${1}"