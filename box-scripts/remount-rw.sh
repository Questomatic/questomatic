#!/bin/bash

SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

source ${SOURCE_DIR}/common_utils.sh

check_root

mount -o remount,rw /