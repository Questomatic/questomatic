#!/bin/bash
set -e
SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $SOURCE_DIR/../

docker run --privileged linuxkit/binfmt:v0.6

docker build -t questomatic:latest .

docker run --privileged -it questomatic:latest /bin/sh -c 'chroot /sysroot /debootstrap/debootstrap --second-stage'
docker commit `docker ps -l --format="{{.ID}}"` questomatic:latest
docker run --privileged -it questomatic:latest /bin/sh -c 'chroot /sysroot apt install \
--allow-unauthenticated -y libc6-dev libjsoncpp-dev libboost-filesystem-dev libboost-regex-dev \
libboost-python-dev libboost-thread-dev libboost-date-time-dev libboost-program-options-dev libssl-dev libz-dev && apt-get clean'
docker commit `docker ps -l --format="{{.ID}}"` questomatic:latest