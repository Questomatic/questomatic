#!/bin/bash
set -e
SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SYSROOT=$SOURCE_DIR/../sysroot
if [ ! -d "$SYSROOT" ]; then

	apt install -y debootstrap qemu-user-static
	debootstrap --foreign --arch=armhf stretch "$SYSROOT" http://deb.debian.org/debian/
	cp /usr/bin/qemu-arm-static "$SYSROOT"/usr/bin/
	cp /etc/resolv.conf "$SYSROOT"/etc/
	chroot "$SYSROOT"/ /debootstrap/debootstrap --second-stage
	chroot "$SYSROOT"/ apt install -y libc6-dev libjsoncpp-dev libboost-filesystem-dev libboost-regex-dev libboost-python-dev libboost-thread-dev libboost-date-time-dev libboost-program-options-dev libssl-dev libz-dev
fi
