#!/bin/bash

SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

source "${SOURCE_DIR}/pretty_print.sh"
source "${SOURCE_DIR}/common_utils.sh"

check_root

DIR=$1

if [ ! -e "$DIR" ]
then
	echo "dir $DIR doesnt't exists"
fi

cd "$DIR"
find . -type f \( ! -wholename ./md5.sum ! -wholename ./MLO ! -wholename ./u-boot.img \) -exec md5sum {} \; > md5.sum
	
