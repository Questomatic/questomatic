FROM ubuntu:18.04

RUN apt update 
RUN apt install -y debootstrap cmake make build-essential binutils-arm-linux-gnueabihf \
gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf
RUN apt install -y debootstrap qemu-user-static

ENV SYSROOT=/sysroot
RUN mkdir ${SYSROOT}
RUN debootstrap --foreign --variant=minbase --arch=armhf stretch "${SYSROOT}" http://deb.debian.org/debian/
RUN cp /usr/bin/qemu-arm-static "${SYSROOT}"/usr/bin/ && cp /etc/resolv.conf "${SYSROOT}"/etc/
RUN apt-get clean

RUN mkdir /build
WORKDIR /build
#COPY . /build


