#!/bin/bash
set -e
SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
IMAGE_NAME=${1:-questomatic:latest}
cd $SOURCE_DIR/../

echo Building image $IMAGE_NAME ....
docker run --privileged linuxkit/binfmt:v0.6

docker build -t $IMAGE_NAME .

echo Running debootstrap second stage ...
docker run --privileged -it $IMAGE_NAME /bin/sh -c 'chroot /sysroot /debootstrap/debootstrap --second-stage'
docker commit `docker ps -l --format="{{.ID}}"` $IMAGE_NAME

echo Installing target dev packages ...
docker run --privileged -it $IMAGE_NAME /bin/sh -c 'chroot /sysroot apt install \
--allow-unauthenticated -y libc6-dev libjsoncpp-dev libboost-filesystem-dev libboost-regex-dev \
libboost-python-dev libboost-thread-dev libboost-date-time-dev libboost-program-options-dev libssl-dev libz-dev && apt-get clean'
docker commit `docker ps -l --format="{{.ID}}"` $IMAGE_NAME