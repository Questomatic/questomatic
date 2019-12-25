#!/bin/bash
OPTIND=1         # Reset in case getopts has been used previously in the shell.

set -e

SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

source "$SOURCE_DIR/config.sh"
source "$SOURCE_DIR/common_utils.sh"
source "$SOURCE_DIR/config.sh"
source "$SOURCE_DIR/pretty_print.sh"

TARGET_DIR=$DEV_NFS_DIR
MFG_BUILD=

function show_help {
    echo -e "$0 [-t <install directory>]\n\
 installs tigerlash to staging dir\n\
-t <dir> override default target dir \"$TARGET_DIR\""
    exit
}

while getopts "h?t:m" opt; do
    case "$opt" in
    h|\?)
        show_help
        exit 0
        ;;
	t)  TARGET_DIR=$OPTARG
    	;;
    m)
        MFG_BUILD=1
        ;;
    esac
done

check_root

TIGERLASH_INSTALL_DIR="$TARGET_DIR"/home/tigerlash

if [ ! -z "$MFG_BUILD" ]; then
	TIGERLASH_OUT_DIR=$MFG_OUT_DIR
fi

if [ ! -d "$TARGET_DIR" ]; then
    pp_error "directory to install tigerlash doesn't exists!"
    exit 1
fi

if [ ! -d "$TIGERLASH_OUT_DIR"  ]
then
    pp_error "Tigerlash is not build yet!"
    exit 1
fi

if [ -z "$MFG_BUILD" ]; then
	sudo rm -rf "$TIGERLASH_INSTALL_DIR/tigerlash" || true
else
	sudo rm -rf "$TIGERLASH_INSTALL_DIR/mfg" || true
fi

sudo cp -a "$TIGERLASH_OUT_DIR" "$TIGERLASH_INSTALL_DIR"

cd "$TIGERLASH_INSTALL_DIR"
sudo find . -exec chown -h $TIGERLASH_OWNER_UID:$TIGERLASH_OWNER_GID {} \; 

sync
pp_step "tigerlash is installed to \"$TIGERLASH_INSTALL_DIR\""
