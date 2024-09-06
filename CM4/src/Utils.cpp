/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */
#include "syshead.h"
#include "Utils.hpp"
#include <ctype.h>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <mutex>
#include "pkt_handler.h"
#include "pico_pkt_temperature.h"
#include "pico_pkt_ping.h"
#include "pico_pkt_fan_pwm.h"
#include "pico_pkt_watchdog.h"
#include "pico_pkt_shutdown.h"
#include "pico_pkt_version.h"

using namespace picod;

char config_file_path[PATH_MAX] = { "" };

int run_cmd(char *cmd, ...)
{
    va_list ap;
    const size_t CMDBUFLEN = 128;
    char buf[CMDBUFLEN];
    va_start(ap, cmd);
    vsnprintf(buf, CMDBUFLEN, cmd, ap);

    va_end(ap);

    return system(buf);
}

uint32_t min(uint32_t x, uint32_t y) {
    return x > y ? y : x;
}

int isRaspberryPi(void)
{
    return run_cmd((char *)"{ grep -qs 'Raspberry Pi' /proc/cpuinfo; } "
     "&& { exit 1; } || { exit 0; }");
}

int isOpenWrt(void)
{
    return run_cmd((char *)"{ grep -qs 'OpenWrt' /etc/os-release; }"
    "&& { exit 1; } || { exit 0; }");
}

int checkLimits(const char * varName, int lowerLimit, int upperLimit, int value)
{
    int retVal = 0;

    if ( (value < lowerLimit) || (value > upperLimit) )
    {
        retVal = 1;
        print_err("Value of %s: %d is outside limits: [%d,%d]\n", 
            varName, value, lowerLimit, upperLimit);        
    }

    return retVal;
}

void set_config_file_path(const char * filePath)
{
    if (filePath) {
        strncpy(config_file_path,filePath,PATH_MAX-1);
    }
}

const char * get_config_file_path(void)
{
    return config_file_path;
}

void print_bytes(const char *msg, const uint8_t *bytes, size_t n)
{
    size_t i;

    if (msg != NULL) {
        puts(msg);
    }

    for (i = 0; i < n; i++) {
        if (i == 0) {
            printf("  0x%02X", bytes[i]);
        } else if ((i + 1) % 8 == 0) {
            printf(" 0x%02X\n ", bytes[i]);
        } else {
            printf(" 0x%02X", bytes[i]);
        }
    }

    putchar('\n');
}

double getSteadyClock() {
    static auto startTime = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = 
        std::chrono::steady_clock::now() - startTime;
    return diff.count();
}

void get_serial_port_settings(struct termios &settings) {
    // Set port settings for canonical input processing
    settings.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    settings.c_iflag = 0;
    settings.c_oflag = 0;
    settings.c_lflag = 0;
    settings.c_cc[VMIN]=1;
    settings.c_cc[VTIME]=0;    
}

int create_virtual_serial_ports(int &fd1, int &fd2)
{
    struct termios settings;
    get_serial_port_settings(settings);
    
    int result = openpty(&fd1, &fd2, NULL, &settings, NULL);
    if (result == -1) {        
        return EXIT_FAILURE;
    } else {
        char *ptty_name = ttyname(fd2);
        if (ptty_name) {
            print_debug("Successfully created: %s", ptty_name);
        }
    }

    return EXIT_SUCCESS;    
}

std::string formatDouble(double value, size_t precision) {
    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss << std::setprecision(precision) << value;
    return oss.str();
}

uint32_t sanitize_watchdog_timeout(uint32_t timeout_seconds){
    timeout_seconds = (timeout_seconds < 1) ? 1 : timeout_seconds;
    timeout_seconds = (timeout_seconds > 0xFFFF) ? 0xFFFF : timeout_seconds;
    return timeout_seconds;
}

Event & getQuitEvent(){
    static Event quitEvent;
    return quitEvent;
}

Event & getWatchdogEvent(){
    static Event theEvent;
    return theEvent;
}

std::vector<std::string> split(const std::filesystem::path aPath) {
    std::vector<std::string> result;
    for (auto it = aPath.begin(); it != aPath.end(); ++it){
        result.push_back(*it);
    }
    return result;
}

auto read_file(std::string_view path) -> std::string {
    constexpr auto read_size = std::size_t(4096);
    auto stream = std::ifstream(path.data());
    stream.exceptions(std::ios_base::badbit);

    if (not stream) {
        throw std::ios_base::failure("file does not exist");
    }
    
    auto out = std::string();
    auto buf = std::string(read_size, '\0');
    while (stream.read(& buf[0], read_size)) {
        out.append(buf, 0, stream.gcount());
    }
    out.append(buf, 0, stream.gcount());
    return out;
}