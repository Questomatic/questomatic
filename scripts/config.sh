
SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

LAUNCH_USER_ID=`id -u`
LAUNCH_USER_GID=`id -g`

#u-boot config
UBOOT_SRC_DIR="$SOURCE_DIR/../u-boot"
UBOOT_OUT_DIR="$SOURCE_DIR/../out/u-boot"
UBOOT_MLO="$UBOOT_OUT_DIR/MLO"
UBOOT_IMG="$UBOOT_OUT_DIR/u-boot.img"
UBOOT_ENV="uEnv.txt"

#kernel config
KERNEL_SRC_DIR="$SOURCE_DIR/../kernel"
KERNEL_OUT_DIR="$SOURCE_DIR/../out/kernel"
KERNEL_INSTALL_DIR="$KERNEL_OUT_DIR/install"
KERNEL_OWNER_UID=0
KERNEL_OWNER_GID=0

#sysroot. if you want to manually build for old rootfs, checkout git@gitlab.flexibity.com:box3-4.x/box3-debian-sysroot.git to the same directory
SYSROOT_DIR="$SOURCE_DIR/../../box3-debian-sysroot/"

#image
IMAGE_DEF_SIZE=350

#rootfs
ROOTFS_DIR="$SOURCE_DIR/../rootfs/rootfs-skel"

#build artifact
TIGERLASH_OUT_DIR="$SOURCE_DIR/../out/questomatic"
TIGERLASH_OWNER_UID=1000
TIGERLASH_OWNER_GID=100

#
DEV_NFS_IP="192.168.1.10"
DEV_NFS_DIR=~/"nfs/rootfs-dev/"
