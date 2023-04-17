cmake_minimum_required (VERSION 3.0)

# Default to CMAKE_BUILD_TYPE = Release unless overridden on command line
# http://www.cmake.org/pipermail/cmake/2008-September/023808.html
if( DEFINED CMAKE_BUILD_TYPE )
    set( CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Set to either \"Release\" or \"Debug\"" )
else()
    set( CMAKE_BUILD_TYPE Release CACHE STRING "Set to either \"Release\" or \"Debug\"" )
endif()

# Get the git repo version number
include(cmake/generate_version.cmake)

project (picod)

message( STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}" ) 

include( GNUInstallDirs )

# add_compile_options(-std=c++17 -Wall)
set( CMAKE_C_FLAGS "-fPIC -Wall ${CMAKE_C_FLAGS}" )
set( CMAKE_CXX_FLAGS "-std=c++17 -fPIC -Wall -Werror ${CMAKE_CXX_FLAGS}" )
set( CMAKE_C_FLAGS_DEBUG   "-g3 -ggdb3" )
set( CMAKE_CXX_FLAGS_DEBUG "-g3 -ggdb3" )

set( XDG_CONFIG_DIRS "/etc/xdg" )
set( TARGET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/" )
set( PROJECT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/" )

add_compile_definitions(GIT_VERSION_STR_POSTFIX="-${GITVER}")
message( STATUS "GIT_VERSION_STR_POSTFIX: ${GITVER}" )

add_executable(picod 
    main.cpp  
    Utils.cpp
    cli.cpp
    i2c.c
    Curl.cpp
    InfluxDB.cpp
    TMP103_I2C.cpp
    pico_pkt_ping.cpp
    pico_pkt_shutdown.cpp
    pico_pkt_fan_pwm.cpp
    pico_pkt_watchdog.cpp
    pico_pkt_temperature.cpp
    pico_pkt_version.cpp
    ubus_server.cpp
    )

# To locate <stdio.h> when building for OpenWRT:
if( DEFINED TOOLCHAIN_DIR )
    #message( STATUS "***TOOLCHAIN_DIR***: ${TOOLCHAIN_DIR}" )
    target_include_directories( picod PUBLIC ${TOOLCHAIN_DIR}/include )
endif()

target_include_directories(picod PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../pico)
target_link_libraries(picod config stdc++ curl ubus ubox)

