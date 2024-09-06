#!/bin/bash

# Creates symbolic links to all picod source files in the 
# OpenWRT build area
srcPath="/home/build/CM4"
dstPath="/home/build/picod"
dstPath2="/home/build/picod/src"
[ ! -d ${srcPath} ] &&\
{ echo "Folder not found: ${srcPath}"; exit 1; }

[ ! -d ${dstPath} ] && { mkdir ${dstPath}; }
[ ! -d ${dstPath2} ] && { mkdir ${dstPath2}; }

[ ! -d ${dstPath}/cmake ] &&\
{ ln -vs "${srcPath}/cmake" "${dstPath}/cmake"; }

for filename in ${srcPath}/../pico/*pkt_*.h; do
    aLink="${dstPath2}/$(basename ${filename})"
    # Remove broken links
    [ -L ${aLink} ] && [ ! -e ${aLink} ] && { rm -v ${aLink}; }    
    # Create the link 
    [ ! -L ${aLink} ] && { ln -vs ${filename} ${aLink}; }   
done

for filename in ${srcPath}/{picod.conf,picod_service,CMakeLists.txt}; do
    aLink="${dstPath}/$(basename ${filename})"
    # Remove broken links
    [ -L ${aLink} ] && [ ! -e ${aLink} ] && { rm -v ${aLink}; }    
    # Create the link 
    [ ! -L ${aLink} ] && { ln -vs ${filename} ${aLink}; }   
done

for filename in ${srcPath}/src/*; do
    aLink="${dstPath2}/$(basename ${filename})"
    # Remove broken links
    [ -L ${aLink} ] && [ ! -e ${aLink} ] && { rm -v ${aLink}; }    
    # Create the link 
    [ ! -L ${aLink} ] && { ln -vs ${filename} ${aLink}; }   
done

# Remove broken links that are only present in the destination
for filename in ${dstPath}/*; do      
    [ -L ${filename} ] && [ ! -e ${filename} ] &&\
    { echo "Broken link "; rm -v ${filename}; }
done
