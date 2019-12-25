#!/bin/bash

set -e

SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

source "${SOURCE_DIR}/pretty_print.sh"
source "${SOURCE_DIR}/common_utils.sh"

check_root

# A POSIX variable
OPTIND=1         # Reset in case getopts has been used previously in the shell.

# Initialize our own variables:
IMG_DIR="non-existent-dir"
IMG_SIZE=
IMG_NAME="rootfs.img"
TMP_MOUNT="mount_temp"
LABEL="box3-rootfs"

#UBOOT_DIR="${SOURCE_DIR}/../u-boot/"
UBOOT_UENV=

PERCENT_EXTRA_SPACE=20

verbose=0

automount_disable

function show_help {
    echo "please fill in the help"
    exit
}

while getopts "h?v:o:s:d:e:n:l:" opt; do
    case "$opt" in
    h|\?)
        show_help
        exit 0
        ;;
    v)  verbose=1
        ;;
    o)  IMG_NAME=$OPTARG
        ;;
    s)  IMG_SIZE=$OPTARG
        ;;
    d)  IMG_DIR=$OPTARG
        ;;
    n)  UBOOT_UENV=$OPTARG
        ;;
    e)  PERCENT_EXTRA_SPACE=$OPTARG
        ;;
	l)  LABEL=$OPTARG
		;;
    esac
done
    
shift $((OPTIND-1))
[ "$1" = "--" ] && shift

pp_info "Settings:\n verbose=$verbose\n img_size=$IMG_SIZEM\n img_name=$IMG_NAME\n img_dir='$IMG_DIR'\n Leftovers: $@\n\n"

if [ ! -e "$IMG_DIR" ]
then
    pp_error "rootfs directory doesn't exists!"
    exit 1
fi


if [ -z "$IMG_SIZE" ]
then
    IMG_SIZE=`du -bs ${IMG_DIR} | cut -f 1`
    IMG_SIZE=$((IMG_SIZE/1048576+1))
    pp_warn "calculating data size = ${IMG_SIZE}"
    IMG_SIZE=$((IMG_SIZE+IMG_SIZE*PERCENT_EXTRA_SPACE*10/1000))
    if [ "$IMG_SIZE" -lt 2 ]
    then
	IMG_SIZE=2
    fi
    pp_warn "calculating image size = ${IMG_SIZE}"
fi

if [ -e "$IMG_NAME" ]
then
    pp_warn "deleting old image"
    rm -rf "$IMG_NAME"
fi

TMP_MOUNT="${IMG_DIR}_$TMP_MOUNT"

if [ -e "$TMP_MOUNT" ]
then
    pp_step "trying to unmount $TMP_MOUNT"
    umount "$TMP_MOUNT" || true
fi

pp_step "creating empty image"

dd if=/dev/zero of="$IMG_NAME" bs=1M count=$IMG_SIZE
chown $LAUNCH_USER_ID:$LAUNCH_USER_GID "$IMG_NAME"

modprobe loop
LOOP_DEV=$(losetup -f)

pp_info "LOOP_DEV = ${LOOP_DEV}"

pp_step "setting up loop device"
losetup "${LOOP_DEV}" "${IMG_NAME}"

pp_step "partitioning device"

(fdisk "${LOOP_DEV}" <<EOF
n
p
1


p
w
EOF
) || true


pp_step "partprobe"
partprobe "${LOOP_DEV}"
pp_step "Creating FS"

pp_step "Make fs with $LABEL label"
mkfs.ext4 "${LOOP_DEV}p1" -L "$LABEL"

pp_step "copying content\n"


pp_step "copying content\n"

if [ -d "$TMP_MOUNT" ]
then
    pp_warn "directory mount_temp exists"
fi

mkdir -p "$TMP_MOUNT" ||true
mount "${LOOP_DEV}p1" "$TMP_MOUNT/"

pp_step "Copying rootfs..."
PWD=`pwd`
cd "${IMG_DIR}"
pp_info "TMP_MOUNT: $TMP_MOUNT\n IMG_DIR:$IMG_DIR"

cp -a . "${TMP_MOUNT}"
cd "$PWD"

UBOOT_UENV_TPL="${UBOOT_DIR}/uEnv/uEnv.txt.${UBOOT_UENV}"

if [ ! -z "${UBOOT_UENV}" ] && [ -e "${UBOOT_UENV_TPL}" ]
then
	pp_info "deploying environment script ${UBOOT_UENV_TPL}"
	cp "${UBOOT_UENV_TPL}" "$TMP_MOUNT"
fi

pp_step "image df"
df | grep "Filesystem\|${LOOP_DEV}"
sync
umount -f "$TMP_MOUNT"
rm -rf "$TMP_MOUNT"

pp_step "Freeing ${LOOP_DEV} \n"

losetup -d "${LOOP_DEV}"

pp_ok "done!"
