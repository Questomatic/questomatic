#!/bin/bash

# A POSIX variable
OPTIND=1         # Reset in case getopts has been used previously in the shell.

set -e

SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

source "${SOURCE_DIR}/pretty_print.sh"
source "${SOURCE_DIR}/common_utils.sh"
source "$SOURCE_DIR/config.sh"

function show_help {
    pp_info "builds questomatic distributable\n -a generate archive"
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

make -j$(nproc) questomatic 24_websocket_srv

ARTIFACT_FULL_NAME=${OUT_DIR}

pp_info "intalling artifacts to \"${ARTIFACT_FULL_NAME}\""

rm -rf "$ARTIFACT_FULL_NAME"
mkdir -p "$ARTIFACT_FULL_NAME"
cd "$SOURCE_DIR/.."

cp -a "$OUT_DIR/../bin/questomatic" "$OUT_DIR/../bin/24_websocket_srv" "$ARTIFACT_FULL_NAME"


if [ ! -z "$ARCHIVE_TL" ]; then
	pp_step "Generating archive"
	cd "$ARTIFACT_FULL_NAME/../"
	tar -czf questomatic-`"${SOURCE_DIR}"/gen-image-id.sh -e -s -d "${SOURCE_DIR}"`.tgz questomatic
fi
