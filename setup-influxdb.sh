#!/bin/bash
#
# Script to setup InfluxDB 2.x for use with the CM4-WRT-A router baseboard.
# Ref: https://hub.docker.com/_/influxdb 
# CM4-WRT-A router baseboard: https://www.amazon.com/dp/B0C17SX8QG
################################################################################
# $0 always point to the shell script name.
basePath=$(realpath $(dirname "$0"))
dataDir=${basePath}/influxdb/data
configDir=${basePath}/influxdb/config

CONTAINER_NAME="influxdb-test"
imageName="influxdb:2.7"

username=picod
pass="vESAgbDhj7bhzAF"
bucket="cm4-wrt-a"
org="MyTechCatalog"

# Initial setup
[ ! -d ${dataDir} ] && { mkdir -p ${dataDir}; }
[ ! -d ${configDir} ] && { mkdir -p ${configDir}; }
[ ! -d ${dataDir} ] && { echo "Folder not found: ${dataDir}"; exit 0; }
[ ! -d ${configDir} ] && { echo "Folder not found: ${configDir}"; exit 0; }

# Get container id from name
theId=`docker ps -aqf "name=^${CONTAINER_NAME}$"`;
[ -z ${theId} ] &&\
{ docker run -d -p 8086:8086 --name ${CONTAINER_NAME} \
      -v ${dataDir}:/var/lib/influxdb2 \
      -v ${configDir}:/etc/influxdb2 \
      -e DOCKER_INFLUXDB_INIT_MODE=setup \
      -e DOCKER_INFLUXDB_INIT_USERNAME=${username} \
      -e DOCKER_INFLUXDB_INIT_PASSWORD=${pass} \
      -e DOCKER_INFLUXDB_INIT_ORG=${org} \
      -e DOCKER_INFLUXDB_INIT_BUCKET=${bucket} \
      -e DOCKER_INFLUXDB_INIT_RETENTION=2w \
      ${imageName}; } && { exit 0; }

# If database container exists, run it.
# Start the database docker container if it is not running
[ "$( docker container inspect -f '{{.State.Running}}' ${CONTAINER_NAME} )" == "true" ] &&\
{ echo "Container named ${CONTAINER_NAME} is already running."; }

[ "$( docker container inspect -f '{{.State.Running}}' ${CONTAINER_NAME} )" == "false" ] &&\
{ docker start "${CONTAINER_NAME}"; }
