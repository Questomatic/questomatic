#!/bin/bash

# A POSIX variable
OPTIND=1         # Reset in case getopts has been used previously in the shell.

set -e

SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

source "${SOURCE_DIR}/pretty_print.sh"
source "${SOURCE_DIR}/common_utils.sh"
source "$SOURCE_DIR/config.sh"

function show_help {
    pp_info "builds tigerlash distributable\n -a generate archive"
    exit
}

OUT_DIR=$TIGERLASH_OUT_DIR
ARCHIVE_TL=
MFG_BUILD=

while getopts "h?am" opt; do
    case "$opt" in
    h|\?)
        show_help
        exit 0
        ;;
	a)
        ARCHIVE_TL=1
        ;;
	m)
        MFG_BUILD=1
        ;;
    esac
done

shift $((OPTIND-1))
[ "$1" = "--" ] && shift

if [ ! -z "$MFG_BUILD" ]; then
	OUT_DIR=$MFG_OUT_DIR
fi

mkdir -p "${OUT_DIR}"
cd "$OUT_DIR/.."

if [ -z "$MFG_BUILD" ]; then
	make -j$(nproc) orange_quest 24_websocket_srv
else
	make -j$(nproc) 28_serial_test
fi

ARTIFACT_FULL_NAME=${OUT_DIR}

pp_info "intalling artifacts to \"${ARTIFACT_FULL_NAME}\""

rm -rf "$ARTIFACT_FULL_NAME"
mkdir -p "$ARTIFACT_FULL_NAME"
cd "$SOURCE_DIR/.."

cp -a scripts/rootfs-updater* "$ARTIFACT_FULL_NAME"
if [ -z "$MFG_BUILD" ]; then
	cp -a configs/tigerlash.json "$OUT_DIR/../bin/orange_quest" "$OUT_DIR/../bin/24_websocket_srv" "$ARTIFACT_FULL_NAME"
else
	cp -a "$OUT_DIR/../bin/"*" $ARTIFACT_FULL_NAME"
	cp -a configs/tigerlash.json "$ARTIFACT_FULL_NAME"/tigerlash.json
fi


if [ ! -z "$ARCHIVE_TL" ]; then
	pp_step "Generating archive"
	cd "$ARTIFACT_FULL_NAME/../"
	tar -czf tigerlash-`"${SOURCE_DIR}"/gen-image-id.sh -e -s -d "${SOURCE_DIR}"`.tgz tigerlash
fi
