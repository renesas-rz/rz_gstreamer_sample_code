#!/bin/bash

# Copyright (c) 2023-2025 Renesas Electronics Corporation and/or its affiliates
# SPDX-License-Identifier: MIT-0

#*******************************************************************************************
#* FILENAME: detect_microphone.sh
#*
#* DESCRIPTION:
#*   Detect 1 microphone at a time.
#*   This script can be used in combination with gst-audiorecord application
#*   Example: ./gst-audiorecord $( ./detect_microphone.sh )
#*
#* AUTHOR: Renesas Electronics Corporation and/or its affiliate       START DATE: 09/09/2024
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
# This variable contains a command to find list of sound cards
# Note that it requires variable ALSA_DEV_FILE to functional properly
CMD_GET_SND="cat $ALSA_DEV_FILE | awk '{ print \$0 }'"

# This variable contains a command to get indices of all sound cards
# Note that it requires variable CMD_GET_SND to functional properly
CMD_GET_SND_INDICES="$CMD_GET_SND | awk '{ print \$1 }'"

# Find sound cards
SND_INDICES="$( eval $CMD_GET_SND_INDICES )"
if [ ! -z "$SND_INDICES" ]
then
    for TEMP_INDEX in $SND_INDICES
    do
        # Check if this sound card has microphone or not?
        HAS_MIC="$( amixer -D hw:$TEMP_INDEX scontrols | grep "Mic" )"

        if [ "$HAS_MIC" != "" ]
        then
            # Suitable sound card is found. It has "Mic" control
            SND_INDEX=$TEMP_INDEX
            DEVICE_NUMBER=${HAS_MIC: -1}
            echo "hw:$SND_INDEX,$DEVICE_NUMBER"

            # Change volume of microphone
            amixer cset name='Aux Switch' on > /dev/null
            amixer cset name='Mixin Left Aux Left Switch' on > /dev/null
            amixer cset name='Mixin Right Aux Right Switch' on > /dev/null
            amixer cset name='ADC Switch' on > /dev/null
            amixer cset name='Mixout Right Mixin Right Switch' off > /dev/null
            amixer cset name='Mixout Left Mixin Left Switch' off > /dev/null
            amixer cset name='Headphone Volume' 50% > /dev/null
            amixer cset name='Headphone Switch' on > /dev/null
            amixer cset name='Mixout Left DAC Left Switch' on > /dev/null
            amixer cset name='Mixout Right DAC Right Switch' on > /dev/null
            amixer cset name='DAC Left Source MUX' 'DAI Input Left' > /dev/null
            amixer cset name='DAC Right Source MUX' 'DAI Input Right' > /dev/null
            amixer sset 'Mic 1 Amp Source MUX' 'MIC_P' > /dev/null
            amixer sset 'Mic 2 Amp Source MUX' 'MIC_P' > /dev/null
            amixer sset 'Mixin Left Mic 1' on > /dev/null
            amixer sset 'Mixin Right Mic 2' on > /dev/null
            amixer sset 'Mic 1' 100% on > /dev/null
            amixer sset 'Mic 2' 100% on > /dev/null
            amixer sset 'Lineout' 100% on > /dev/null
            amixer set 'Headphone' 100% on > /dev/null
            amixer set 'DVC In',0 100% > /dev/null
            amixer cset name='Mixin PGA Volume' 2 > /dev/null
            amixer cset name='Mixin PGA Switch' on > /dev/null
            amixer cset name='ADC Volume' 67% > /dev/null
	    break
        fi
    done
fi
