#!/bin/bash

set -e

SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

source "$SOURCE_DIR/config.sh"
source "$SOURCE_DIR/common_utils.sh"
source "${SOURCE_DIR}/pretty_print.sh"

OPTIND=1         # Reset in case getopts has been used previously in the shell.

SRC_IMG=
TARGET_IMG=
IMG_SIZE=
SINGLE=

#Temporary file for the script
TMP=/tmp/disk_wipe

#Destination device for suppressed output
NULL=/dev/null

PID_DD=

#Clean up after ourselves
function clean_up() {
	#Give us a new line
	echo
	#Kill the dd command, if it isn't already stopped
	if [ ! -z "$PID_DD" ]; then
		echo -n "Waiting for dd pid $PID_DD to exit"
		while kill -s ABRT $PID_DD 2>&1 >$NULL
		do
			echo -n .
			sleep 1
		done
	fi
	rm -rf $TMP 2>&1 >$NULL || true
	echo
	#Aaaaand we're done
	exit
}

trap clean_up SIGHUP SIGINT SIGTERM

function show_help {
    echo -e "copies an image from src to dst.\n\
usage: $0 -s <source image> -t <target image> [-1]\n\
-s can be block device or image\n\
-t can be block device or image\n\
-1 disable loop -t monitoring"
    exit
}

function wait_media {
	pp_warn "Ready to burn image \"$SRC_IMG\" to \"$TARGET_IMG\". Insert drive"
	while [ ! -e $1 ]; do
		echo -n .
		sleep .3
	done
	echo
	sleep .5
}

function wait_media_eject {
	pp_step "Remove the drive"
	while [ -e $1 ]; do
		echo -n .
		sleep .3
	done
	echo	
}

function wait_dd() {
	pp_step "Monitoring dd with pid $PID_DD"
	sleep .2
	while kill -s 0 $1 > $NULL 2>&1
	do
		echo -en "\e[32m.\e[0m"
		sleep 0.25
	done
	PROGRESS="`cat $TMP`"

	if ! grep -q "$IMG_SIZE" "$TMP"; then
		PROGRESS=
		echo
		pp_error "Image was not copied completely! expected \"$IMG_SIZE\""
	fi
	
	if [ -z "$PROGRESS" ]; then
		pp_error "Error msg:\n \e[91m`cat $TMP`\e[0m\n"
		cp $NULL $TMP || true
		PID_DD=
		return 1
	else
		if [ ! -z "$PROGRESS" ]; then
			#Output progress, overwriting the previous progress line
			echo -e "\n$PROGRESS"
		else
			echo -en "\n`cat $TMP`"
		fi
	fi
	
	cp $NULL $TMP || true
	PID_DD=
	echo
}

while getopts "h?t:s:1" opt; do
    case "$opt" in
    h|\?)
        show_help
        exit 0
        ;;
    t)  TARGET_IMG=$OPTARG
        ;;
    s)  SRC_IMG=$OPTARG
        ;;
	1)  SINGLE=1
        ;;
    esac
done

check_root

if [ ! -f "$SRC_IMG" ]
then 
	pp_error "source image \"$SRC_IMG\" doesn't exists"
	exit 1
fi

if [ -z "$TARGET_IMG" ]; then
	TARGET_IMG=/dev/sdb
	pp_warn "using $TARGET_IMG as default" 
fi

if mount|grep -q "$TARGET_IMG"; then
    pp_error "$TARGET_IMG is mounted! refusing to write"
    exit 1
fi

pp_step "Starting flash cycle"

IMG_SIZE=$(stat -c%s "$SRC_IMG")

while :; do
	wait_media $TARGET_IMG
	pp_step "Writing image"
	touch $TMP
	dd if="$SRC_IMG" of="$TARGET_IMG" bs=1M > $TMP 2>&1 & PID_DD=$!

	#Keep an eye on our progress
	if [ -z "$SINGLE" ]; then
		wait_dd $PID_DD || true
	else
		wait_dd $PID_DD
		break;
	fi
	wait_media_eject $TARGET_IMG
done
