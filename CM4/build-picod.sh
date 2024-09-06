#!/bin/bash
#
# Build picod for the the CM4-WRT-A baseboard. Picod is a service 
# for monitoring the Raspberry Pi Pico on the CM4-WRT-A baseboard: 
# https://www.tindie.com/products/mytechcatalog/rpi-cm4-router-baseboard-with-nvme/
################################################################################
CONTAINER_NAME=rpi-picod-build
imageName="cross-slim:latest"

scripDir=$(dirname "$0") 
basePath=$(realpath "${scripDir}");
GITVER="$(git -C ${basePath} describe --long --tags --dirty --always)";

theId=`docker ps -aqf "name=^${CONTAINER_NAME}$"`;
[ -z ${theId} ] &&\
{ docker run -t -d --name ${CONTAINER_NAME} \
    -e GITVER=${GITVER} \
    -v "${basePath}"/../CM4/:/home/build/CM4 \
    -v "${basePath}"/../pico/:/home/build/pico:ro ${imageName}; } &&\
{ docker exec -w /home/build/CM4 -it ${CONTAINER_NAME} make package; } &&\
{ docker exec -w /home/build/CM4 -e GITVER=${GITVER} -it ${CONTAINER_NAME} mv ../picod_1.0_arm64.deb ./build/; exit 0; }

[ "$( docker container inspect -f '{{.State.Running}}' ${CONTAINER_NAME} )" == "true" ] &&\
{ docker exec -w /home/build/CM4 -e GITVER=${GITVER} -it ${CONTAINER_NAME} make package; } &&\
{ docker exec -w /home/build/CM4 -e GITVER=${GITVER} -it ${CONTAINER_NAME} mv ../picod_1.0_arm64.deb ./build/; exit 0;}

[ "$( docker container inspect -f '{{.State.Running}}' ${CONTAINER_NAME} )" == "false" ] &&\
{ echo -e "Starting \033[1;36m${CONTAINER_NAME}\033[0m" && docker start ${CONTAINER_NAME}; } &&\
{ docker exec -w /home/build/CM4 -e GITVER=${GITVER} -it ${CONTAINER_NAME} make package; } &&\
{ docker exec -w /home/build/CM4 -e GITVER=${GITVER} -it ${CONTAINER_NAME} mv ../picod_1.0_arm64.deb ./build/; exit 0;}

