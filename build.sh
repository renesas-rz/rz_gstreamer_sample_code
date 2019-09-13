#!/usr/bin/env bash

LINUX_DIR=./linux
GST_DIR=./gst
QT_DIR=./qt
OUTPUT_DIR=''
MANIFEST_FILE='app.manifest'

build_linux()
{
	cd $LINUX_DIR
	for DIR in *
	do
		cd $DIR
		make clean;make
		cd ../
		echo "${LINUX_DIR}/${DIR}/${DIR}" >> ../${MANIFEST_FILE}
	done
	cd ../
}

build_gst()
{
	cd $GST_DIR
	# app dir = app name
	for DIR in *
	do
		cd $DIR
		make clean;make
		cd ../
		echo "${GST_DIR}/${DIR}/${DIR}" >> ../${MANIFEST_FILE}
	done
	cd ../
}

build_qt()
{
	cd $QT_DIR
	# app dir = app name
	for DIR in *
	do
		cd $DIR
		make clean;qmake;make
		cd ../
		echo "${QT_DIR}/${DIR}/${DIR}" >> ../${MANIFEST_FILE}
	done
	cd ../
}


copy_to_output()
{

	echo "Copy to $OUTPUT_DIR ..."

	if [ ! -d $OUTPUT_DIR ];
	then 
		mkdir $OUTPUT_DIR
	fi

	while read line; do
		# reading each line
		echo "Copy app $line to output folder... "
		cp $line $OUTPUT_DIR -f
	done < $MANIFEST_FILE
}

# Input 1 is output folder
if [ -z "$1" ];
then 
	OUTPUT_DIR='./output'
else
	OUTPUT_DIR=$1
fi


# Create manifest files
if [ ! -e ${MANIFEST_FILE} ];
then
	touch $MANIFEST_FILE
else
	rm $MANIFEST_FILE
	touch $MANIFEST_FILE
fi

# Build all app
build_linux
#build_gst
#build_qt

# Then copy to output folder
copy_to_output

echo "Finish! Please get the built app in folder $OUTPUT_DIR"
