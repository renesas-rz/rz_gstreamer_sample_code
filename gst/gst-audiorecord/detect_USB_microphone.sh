#!/bin/bash

#*******************************************************************************************
#* FILENAME: detect_USB_microphone.sh
#*
#* DESCRIPTION:
#*   Detect 1 USB microphone at a time.
#*   This script can be used in combination with gst-audiorecord application
#*   Example: ./gst-audiorecord $( ./detect_USB_microphone.sh )
#*
#* AUTHOR: RVC       START DATE: 02/18/2020
#*
#* CHANGES:
#*
#*******************************************************************************************

# According to website "https://en.wikibooks.org/wiki/Configuring_Sound_on_Linux/HW_Address":
# ---
# By viewing the virtual file /proc/asound/cards your kernel will show you the
# device names for the cards it is has ready for use.
# If your card isn't listed here then your kernel does not support it.
ALSA_DEV_FILE="/proc/asound/cards"

# This variable contains a command to find list of USB sound cards
# Note that it requires variable ALSA_DEV_FILE to functional properly
CMD_GET_USB_SND="cat $ALSA_DEV_FILE | awk '/[0-9]+ \\[[[:print:]]+\\]: USB-Audio - [[:print:]]+/ { print \$0 }'"

# This variable contains a command to get indices of all USB sound cards
# Note that it requires variable CMD_GET_USB_SND to functional properly
CMD_GET_USB_SND_INDICES="$CMD_GET_USB_SND | awk '{ print \$1 }'"

# Find USB sound cards
USB_SND_INDICES="$( eval $CMD_GET_USB_SND_INDICES )"
if [ ! -z "$USB_SND_INDICES" ]
then
    for TEMP_INDEX in $USB_SND_INDICES
    do
        # Check if this USB sound card has microphone or not?
        HAS_MIC="$( amixer -D hw:$TEMP_INDEX scontrols | grep "Mic" )"

        if [ "$HAS_MIC" != "" ]
        then
            # Suitable USB sound card is found. It has "Mic" control
            USB_SND_INDEX=$TEMP_INDEX
            DEVICE_NUMBER=${HAS_MIC: -1}
            echo "hw:$USB_SND_INDEX,$DEVICE_NUMBER"

            # Change volume of microphone
            amixer -D hw:$USB_SND_INDEX set "Mic" 100% > /dev/null

            break
        fi
    done
fi
