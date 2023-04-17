cmake_minimum_required(VERSION 3.16)

# Pull in SDK (must be before project)
include(cmake/pico_sdk_import.cmake)
# Get the git repo version number
include(cmake/generate_version.cmake)

project(cm4-wrt-a)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )
add_compile_definitions(GIT_VERSION_STR_POSTFIX="-${GITVER}")
message( STATUS "GIT_VERSION_STR_POSTFIX: ${GITVER}" )

add_executable(cm4-wrt-a
        cm4-wrt-a.c
        pico_pkt_ping.c
        pico_pkt_temperature.c
        pico_pkt_fan_pwm.c
        pico_pkt_watchdog.c
        pico_pkt_shutdown.c
        pico_pkt_version.c
        )

target_include_directories(cm4-wrt-a PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# pull in common dependencies
target_link_libraries(cm4-wrt-a 
        pico_stdlib 
        hardware_adc 
        hardware_pwm
        )

# create map/bin/hex file etc.
pico_add_extra_outputs(cm4-wrt-a)
