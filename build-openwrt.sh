#!/bin/bash
#
# Build OpenWrt image for the the CM4-WRT-A baseboard. It also builds a service 
# named picod, which is a tool for monitoring the Raspberry Pi Pico on the 
# CM4-WRT-A baseboard: https://www.amazon.com/dp/B0C17SX8QG
################################################################################
git_url="https://git.openwrt.org/openwrt/openwrt.git"
[ "x$1" == "x" ] && { branch=$(git ls-remote --tags ${git_url} 'refs/tags/v*' |\
grep '[^\{\}]$' | tail -n 1 | awk '{print $2}' | cut -d'/' -f3); } ||\
{ branch="$1"; }
echo -e "Using OpenWrt tag: \033[1;36m${branch}\033[0m"
CONTAINER_NAME=openwrt-build
imageName="openwrt:${branch}"
# $0 always point to the shell script name.
scripDir=$(dirname "$0") 
basePath=$(realpath "${scripDir}");
dockerFile=${basePath}/OpenWrtDockerfile

[ ! -d "${basePath}/bin" ] && { mkdir "${basePath}/bin"; }

theId=`docker ps -aqf "name=^${CONTAINER_NAME}$"`;
[ -z ${theId} ] && { printf "#This file is auto-generated.\nFROM debian:stable-slim\n\n" > ${dockerFile}; } &&\
{ printf "RUN useradd build -u $(id -u) -m -c 'OpenWrt Builder'\n" >> ${dockerFile}; } &&\
{ printf "RUN apt-get update\n" >> ${dockerFile}; } &&\
{ printf "RUN apt install -y build-essential clang \\" >> ${dockerFile}; } &&\
{ printf "\n\tflex bison g++ gawk gcc-multilib g++-multilib \\" >> ${dockerFile}; } &&\
{ printf "\n\tgettext git libncurses5-dev libssl-dev \\" >> ${dockerFile}; } &&\
{ printf "\n\tpython3-distutils rsync unzip zlib1g-dev file wget\n" >> ${dockerFile}; } &&\
{ printf "USER build\n" >> ${dockerFile}; } &&\
{ printf "RUN mkdir ~/openwrt ~/picod\n" >> ${dockerFile}; } &&\
{ printf "WORKDIR /home/build/openwrt\n" >> ${dockerFile}; } &&\
{ printf "RUN git clone -b ${branch} ${git_url} .\n" >> ${dockerFile}; } &&\
{ printf "RUN make distclean && ./scripts/feeds update -a && ./scripts/feeds install -a\n" >> ${dockerFile}; } &&\
{ printf "RUN echo '/home/build/CM4/create_picod_links.sh' > ~/build-openwrt.sh\n" >> ${dockerFile}; } &&\
{ printf "RUN echo 'cp -r /home/build/CM4/package /home/build/openwrt/' >> ~/build-openwrt.sh\n" >> ${dockerFile}; } &&\
{ printf "RUN echo 'cp /home/build/CM4/diffconfig /home/build/openwrt/.config' >> ~/build-openwrt.sh\n" >> ${dockerFile}; } &&\
{ printf "RUN echo 'make defconfig && make tools/install -j\$(nproc) && make toolchain/install -j\$(nproc)' >> ~/build-openwrt.sh\n" >> ${dockerFile}; } &&\
{ printf "RUN echo 'cp /home/build/CM4/config.txt ./target/linux/bcm27xx/image/config.txt' >> ~/build-openwrt.sh\n" >> ${dockerFile}; } &&\
{ printf "RUN echo 'make -j\$(nproc) defconfig download clean world' >> ~/build-openwrt.sh\n" >> ${dockerFile}; } &&\
{ printf "RUN chmod +x ~/build-openwrt.sh\n" >> ${dockerFile}; } &&\
{ docker build ${basePath}/ -f ${dockerFile} -t ${imageName}; } &&\
{ docker run -t -d --name ${CONTAINER_NAME} \
    -v "${basePath}"/CM4/:/home/build/CM4:ro \
    -v "${basePath}"/pico/:/home/build/pico:ro \
    -v "${basePath}"/bin/:/home/build/openwrt/bin ${imageName}; } &&\
{ docker exec -it ${CONTAINER_NAME} bash -c "/home/build/build-openwrt.sh"; exit 0;}

[ "$( docker container inspect -f '{{.State.Running}}' ${CONTAINER_NAME} )" == "true" ] &&\
{ docker exec -it ${CONTAINER_NAME} bash; exit 0; }

[ "$( docker container inspect -f '{{.State.Running}}' ${CONTAINER_NAME} )" == "false" ] &&\
{ echo -e "Starting \033[1;36m${CONTAINER_NAME}\033[0m" && docker start -i ${CONTAINER_NAME}; exit 0; }

# within the Docker container
#make package/picod/{clean,compile} -j$(nproc)
