/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef UTILS_H
#define UTILS_H
#include <cstdint>
#include "syshead.h"
#include <pty.h>
#include "settings.hpp"
#include "pkt_handler.h"
#include <iostream>
#include <filesystem>
#include <sstream>
#include <vector>
#include <deque>
#include <complex>
#include "Event.hpp"

#define BAUDRATE B115200

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#ifndef ARRAY_SIZE
#   define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

#define BOOLEAN_TO_STR(value) value ? "True" : "False"

#define print_debug(str, ...)                       \
    printf(str" - %s:%u\n", ##__VA_ARGS__, __FILE__, __LINE__);

#define print_err(str, ...)                     \
    fprintf(stderr, str, ##__VA_ARGS__);

void print_bytes(const char *msg, const uint8_t *bytes, size_t n);

int run_cmd(char *cmd, ...);
uint32_t min(uint32_t x, uint32_t y);
int isRaspberryPi(void);
int isOpenWrt(void);
void set_config_file_path(const char * filePath);
const char * get_config_file_path(void);
int checkLimits(const char * varName, int lowerLimit, 
    int upperLimit, int value);
double getSteadyClock();

std::string formatDouble(double value, size_t precision = 1);

uint32_t sanitize_watchdog_timeout(uint32_t timeout_seconds);

std::vector<std::string> split(const std::filesystem::path aPath);
auto read_file(std::string_view path) -> std::string;

Event & getQuitEvent();
Event & getWatchdogEvent();

/// @brief Returns the version string of the firmware on the RPi Pico
/// @param fd RPi Pico serial port file descriptor
/// @param picoVersion Version string
void get_pico_version(std::string & picoVersion);

/// @brief This function creates a virtual serial port and
/// returns file descriptors to each end of the port
/// @param fd1 RPi Pico serial port file descriptor
/// @param fd2 Host (CM4) serial port file descriptor
int create_virtual_serial_ports(int &fd1, int &fd2);

void get_serial_port_settings(struct termios &settings);


#endif
