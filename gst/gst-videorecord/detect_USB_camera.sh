#!/bin/bash

unset camera_dev

for I in /sys/class/video4linux/*;
do
	if [ "`cat $I/name | egrep "UVC Camera"`" ]
	then
		camera_dev=`echo $I | sed 's/\/sys\/class\/video4linux/\/dev/'`
		echo $camera_dev
		break
	fi
done
if [ -z $camera_dev ]
then
	echo "Cannot find USB camera !"
fi
