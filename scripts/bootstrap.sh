#!/bin/bash
SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

set -e

source "${SOURCE_DIR}/pretty_print.sh"
source "${SOURCE_DIR}/common_utils.sh"
source "$SOURCE_DIR/config.sh"

function show_help {
    echo "use -r <path_to_sysroot> to provide sysroot"
    echo "use -r \"\" to use system sysroot"
    echo "use -g to build debug"
    echo "use -f to force cmake config rebuild"
    exit
}

OUT_DIR="$SOURCE_DIR/../out"
BOOTSTRAP_FLAG="$OUT_DIR/.bootstrap"
BOOTSTRAP_SYSROOT=
FORCE=
DEBUG=
SYSROOT=sysroot

OPTIND=1

while getopts ":h?r:fg" opt; do
    case "$opt" in
    h)
        show_help
        exit 0
        ;;
    r)  SYSROOT=$OPTARG
    	;;
    g)  DEBUG=1
    	;;
	f)  FORCE=1
        ;;
    esac
done

shift "$((OPTIND - 1))"

mkdir -p "$OUT_DIR"
cd "$SOURCE_DIR/../out"

if [[ ${SYSROOT:0:1} != "/" ]] ; then 
SYSROOT=`pwd`/../$SYSROOT 
fi

if [ ! -z "$SYSROOT" ]; then
	check_dir "$SYSROOT"
	SYSROOT=$COMMON_UTILS_CHECK_DIR
fi

if [ -e "$BOOTSTRAP_FLAG" ]
then
	read BOOTSTRAP_SYSROOT < "${BOOTSTRAP_FLAG}"
fi

pp_info "\
BOOTSTRAP_SYSROOT: $BOOTSTRAP_SYSROOT\n\
SYSROOT: $SYSROOT\
"

if [ "$SYSROOT" != "$BOOTSTRAP_SYSROOT" ]
then
	FORCE=1
	pp_step "Sysroot has changed from \"$BOOTSTRAP_SYSROOT\" to \"$SYSROOT\". Forcing rebuild"
	echo "$SYSROOT" > "${BOOTSTRAP_FLAG}"
fi

if [ ! -z "$FORCE" ]
then
	make clean ||true
	rm -r CMakeFiles || true
	rm CMakeCache.txt || true
fi

pp_info "Passing additional options to cmake: $@"

DEBUG_ARG=
if [ ! -z "$DEBUG" ]; then
	DEBUG_ARG="-DCMAKE_BUILD_TYPE=Debug"
fi

if [ ! -z "$SYSROOT" ]
then
    cmake -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake -DTRGT=arm -DTCPREFIX="arm-linux-gnueabihf" -DTCVERSION="6" -DSYSROOT="$SYSROOT" "$DEBUG_ARG" $@ ..
else
    cmake "$DEBUG_ARG" CMakelists.txt $@ ..
fi
