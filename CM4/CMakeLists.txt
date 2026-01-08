cmake_minimum_required (VERSION 3.5)

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

include( GNUInstallDirs )

# add_compile_options(-std=c++17 -Wall)
set( CMAKE_C_FLAGS "-fPIC -Wall ${CMAKE_C_FLAGS}" )
set( CMAKE_CXX_FLAGS "-std=c++17 -fPIC -Wall -Werror ${CMAKE_CXX_FLAGS}" )
set( CMAKE_C_FLAGS_DEBUG   "-g3 -ggdb3" )
set( CMAKE_CXX_FLAGS_DEBUG "-g3 -ggdb3" )

set( XDG_CONFIG_DIRS "/etc/xdg" )
set( TARGET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/" )
set( PROJECT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/" )

add_compile_definitions(FMT_HEADER_ONLY=1)
add_compile_definitions(GIT_VERSION_STR_POSTFIX="-${GITVER}")
message( STATUS "GIT_VERSION_STR_POSTFIX: ${GITVER}" )

set (PICOD_SRCS
    src/main.cpp
    src/Utils.cpp
    src/cli.cpp
    src/i2c.c
    src/InfluxDB.cpp
    src/TMP103_I2C.cpp
    src/pico_pkt_ping.cpp
    src/pico_pkt_shutdown.cpp
    src/pico_pkt_fan_pwm.cpp
    src/pico_pkt_watchdog.cpp
    src/pico_pkt_temperature.cpp
    src/pico_pkt_version.cpp
    src/Event.cpp
    src/PacketHandler.cpp
    )

set (PICOD_EXTRA_SRCS
    src/WebServer.cpp
    src/SSEDispatcher.cpp    
    src/NLTemplate/NLTemplate.cpp
    )

# Building OpenWRT
if( DEFINED TOOLCHAIN_DIR )
    message( STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}" )

    add_executable(picod 
        ${PICOD_SRCS}
        src/ubus_server.cpp
    )
    #message( STATUS "***TOOLCHAIN_DIR***: ${TOOLCHAIN_DIR}" )
    # To locate <stdio.h> when building for OpenWRT:
    target_include_directories( picod PUBLIC 
        ${TOOLCHAIN_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../pico
    )

    target_link_libraries(picod config stdc++ ubus ubox )
else() # Building standalone picod
    find_package(PkgConfig REQUIRED)
        
    add_compile_definitions(NO_UBUS=1)

    message( STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}" )
    
    add_executable(picod 
        ${PICOD_SRCS}
        ${PICOD_EXTRA_SRCS}
    )

    add_executable(pico-cli 
        src/client.cpp
    )
    
    target_include_directories( picod PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/NLTemplate
        ${CMAKE_CURRENT_SOURCE_DIR}/src/fmt
        ${CMAKE_CURRENT_SOURCE_DIR}/../pico
    )

    target_include_directories( pico-cli PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/fmt
    )

    target_link_libraries(picod libconfig.a stdc++)

    target_link_libraries(pico-cli stdc++)
endif()
