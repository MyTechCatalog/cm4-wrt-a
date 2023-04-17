#!/bin/bash
#
# Build Raspberry Pi Pico firmware for the CM4-WRT-A baseboard: 
# https://www.amazon.com/dp/B0C17SX8QG
################################################################################
CONTAINER_NAME=pico-build
imageName="pico-image"
# $0 always point to the shell script name.
scripDir=$(dirname "$0") 
basePath=$(realpath "${scripDir}");
dockerFile=${basePath}/PicoDockerfile
picoDir="/home/build/pico"
binDir="${picoDir}/build"
PICO_SDK_PATH="/home/build/pico-sdk"
GITVER="$(git -C ${basePath} describe --long --tags --dirty --always)";

theId=`docker ps -aqf "name=^${CONTAINER_NAME}$"`;
[ -z ${theId} ] && { printf "#This file is auto-generated.\nFROM debian:stable-slim\n\n" > ${dockerFile}; } &&\
{ printf "RUN useradd build -u $(id -u) -m -c 'RPi Pico Builder'\n" >> ${dockerFile}; } &&\
{ printf "RUN apt-get update\n" >> ${dockerFile}; } &&\
{ printf "RUN apt install -y git cmake gcc-arm-none-eabi \\" >> ${dockerFile} ; }&&\
{ printf "\n\tlibnewlib-arm-none-eabi build-essential nano python3-dev\n" >> ${dockerFile}; } &&\
{ printf "USER build\n" >> ${dockerFile}; } &&\
{ printf "WORKDIR /home/build\n" >> ${dockerFile}; } &&\
{ printf "RUN git clone https://github.com/raspberrypi/pico-sdk.git ~/pico-sdk\n" >> ${dockerFile}; } &&\
{ printf "RUN git -C ~/pico-sdk submodule update --init\n" >> ${dockerFile}; } &&\
{ docker build ${basePath}/ -f ${dockerFile} -t ${imageName}; } &&\
{ docker run -t -d --name ${CONTAINER_NAME} \
    -e PICO_SDK_PATH=${PICO_SDK_PATH} \
    -e GITVER=${GITVER} \
    -v "${basePath}"/pico/:${picoDir} ${imageName}; } &&\
{ docker exec -it -w ${picoDir} ${CONTAINER_NAME} \
  bash -c "mkdir ${binDir}; cd ${binDir} && cmake /home/build/pico/ && make"; exit 0; }

[ "$( docker container inspect -f '{{.State.Running}}' ${CONTAINER_NAME} )" == "true" ] &&\
{ echo -e "Attaching to \033[1;36m${CONTAINER_NAME}\033[0m"; } && \
{ docker exec -e GITVER=${GITVER} -it -w ${picoDir} ${CONTAINER_NAME} bash; exit 0; }

[ "$( docker container inspect -f '{{.State.Running}}' ${CONTAINER_NAME} )" == "false" ] &&\
{ echo -e "Starting \033[1;36m${CONTAINER_NAME}\033[0m" && \
  docker start ${CONTAINER_NAME} && docker exec -it \
    -e PICO_SDK_PATH=${PICO_SDK_PATH} \
    -e GITVER=${GITVER} -w ${picoDir} ${CONTAINER_NAME} bash; }
