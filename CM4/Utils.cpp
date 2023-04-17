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

ssize_t send_serial_port_command(int fd, const uint8_t * buf, size_t length){
    
    int result = tcflush(fd, TCIFLUSH);
    if (result == -1) {
        print_err("Failed to flush serial port: %s\n", strerror(errno));
    }
       
    ssize_t bytesWritten = write(fd, buf, length);
    
    if (bytesWritten != (ssize_t)length) {
       print_err("Error writing to serial port: %s\n", strerror(errno));
    }

    return bytesWritten;
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

bool get_pico_response(int fd, pkt_buf &pkt, uint8_t pkt_id) {
    int maxfd; // maximum file desciptor used
    fd_set readfds; // file descriptor set    
    maxfd = fd + 1;  // maximum bit entry (fd) to test

    bool success = true;

    // clear the socket set 
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    
    struct timeval timeout;
    // set timeout value
    timeout.tv_usec = 0;  // Microseconds
    timeout.tv_sec  = 1;  // Seconds
    
    // Wait for a reply
    int res = select(maxfd, &readfds, NULL, NULL, &timeout);
    if (res == -1) {
        // Handle error.
        print_err("select() failed on serial port: %s\n", strerror(errno));
        success = false;
    } else if ((res > 0) && (FD_ISSET(fd, &readfds))) {
        // message from pico to host is available
        size_t numBytesToRead = PICO_PKT_LEN;

        res = read(fd, (void*)&pkt.resp, numBytesToRead);
                
        if (res == -1) {
            print_err("Host failed to read serial port: %s", strerror(errno));
            success = false;
        } else if ((res != PICO_PKT_LEN) || (pkt.resp[PKT_MAGIC_IDX] != pkt_id)) {               
            success = false;
        } else {
            success = true;
        }
    }

    return success;
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