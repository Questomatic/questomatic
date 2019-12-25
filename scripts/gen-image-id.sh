#!/bin/bash

SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

source "${SOURCE_DIR}/pretty_print.sh"
source "${SOURCE_DIR}/common_utils.sh"

OPTIND=1         # Reset in case getopts has been used previously in the shell.

SOURCE_DIR="$( pwd )"

function show_help {
    echo "please fill in the help"
    exit
}

IMAGE_DIR=
IMAGE_ID=
IMAGE_ID_DIR=
ECHO=
SHORT=

while getopts "h?d:f:t:es" opt; do
    case "$opt" in
    h|\?)
        show_help
        exit 0
        ;;
	d)  IMAGE_DIR=$OPTARG
        ;;
    f)  IMAGE_ID=$OPTARG
        ;;
    t)  IMAGE_ID_DIR=$OPTARG
        ;;
    e)  ECHO=1
    	;;
    s)  SHORT=1
        ;;
    esac
done
    
shift $((OPTIND-1))
[ "$1" = "--" ] && shift

if [ -z "$IMAGE_ID" ]
then
    IMAGE_ID="image.id"
fi

if [ -z "$IMAGE_DIR" ]
then
    #echo "please provide image dir"
    #exit 1
	IMAGE_DIR=`pwd`
fi

#echo "img_id: $IMAGE_ID_DIR"

if [ ! -z "$IMAGE_ID_DIR" ]
then
	cd "$IMAGE_ID_DIR"
	#IMAGE_ID_DIR=$( (cd "$IMAGE_ID_DIR") && pwd)
	IMAGE_ID_DIR="$( pwd )/"
fi

#echo $IMAGE_ID_DIR

cd "$IMAGE_DIR"

IMAGE_DIR=$( pwd )

ARTIFACT_NAME="$(git rev-parse --abbrev-ref HEAD)-r$(git rev-list --count HEAD)-$(git rev-parse --short HEAD)$(evil_git_dirty)"
ARTIFACT_NAME="${ARTIFACT_NAME/"/"/"_"}"

#echo "image id dir: $IMAGE_ID_DIR"

if [ -z "$ECHO" ]
then
	echo -e "\
Generated from: $IMAGE_DIR\n\
Revision: $ARTIFACT_NAME" > "$IMAGE_ID_DIR$IMAGE_ID"
else
	if [ -z "$SHORT" ]
	then
		echo -e "$IMAGE_DIR;$ARTIFACT_NAME"
	else
		echo -e "$ARTIFACT_NAME"
	fi
fi
