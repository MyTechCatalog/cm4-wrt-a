
/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <string.h>
#include <chrono>
#include <deque>
#include <functional>
#include "syshead.h"
#include "Utils.hpp"
#include "cli.hpp"
#include "pkt_handler.h"
#include "pico_pkt_temperature.h"
#include "pico_pkt_ping.h"
#include "pico_pkt_fan_pwm.h"
#include "pico_pkt_watchdog.h"
#include "pico_pkt_shutdown.h"
#include "pico_pkt_version.h"
#include "SensorID.hpp"
#include "ubus_server.hpp"
#include "TMP103_I2C.hpp"


using namespace picod;

picod::Settings appSettings;

/// @brief Initializes the Raspberry Pi Pico with settings from the 
/// configuration file
/// @param pico_fd Pico serial port file descriptor
/// @return Exit code: 0 - Success, 1 - Failure.
int init_pico(int &pico_fd);

int main(int argc, char** argv)
{    
    int retVal  = EXIT_SUCCESS;
    pico_fd = 0;

    openlog("picod", LOG_PID, LOG_USER);

    parse_cli(argc, argv);

    if(strlen(get_config_file_path()) == 0) {
        usage(argv[0]);
        retVal = EXIT_FAILURE;
        goto cleanup;
    }
    else if (parse_config_file(get_config_file_path()) != 0) {
        usage(argv[0]);
        retVal = EXIT_FAILURE;
        goto cleanup;
    }
    
    if ((retVal = init_pico(pico_fd)) == EXIT_SUCCESS) {
        retVal = run_ubus_server();
    }

cleanup:
    close(pico_fd);
    closelog();
    
    return retVal;
}

int init_pico(int &fd) {    
    int retVal  = EXIT_SUCCESS;
    int result = 0;    
        
    // open the device to be non-blocking (read will return immediately)
    fd = open(appSettings.pico_serial_device_path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {       
        print_err("Failed to open: %s, %s\n", 
            appSettings.pico_serial_device_path.c_str(), strerror(errno));
        
        syslog(LOG_DEBUG, "Failed to open: %s, %s", 
            appSettings.pico_serial_device_path.c_str(), strerror(errno));
        retVal  = EXIT_FAILURE; 
        goto cleanup; 
    }

    struct termios settings;    
    get_serial_port_settings(settings);
    
    result = tcflush(fd, TCIFLUSH);
    if (result == -1) {
        print_err("Failed to flush serial port: %s\n", strerror(errno));            
        syslog(LOG_DEBUG, "Failed to flush serial port: %s", strerror(errno));
    }
    
    result = tcsetattr(fd, TCSANOW, &settings);
    if (result == -1) {
        print_err("Failed to apply serial port settings: %s\n", strerror(errno));            
        syslog(LOG_DEBUG, "Failed to apply serial port settings: %s", strerror(errno));
        retVal  = EXIT_FAILURE; 
        goto cleanup;
    }    
    
    TMP103_I2C::instance();

    get_pico_version(fd, picoVersion);
    if (!picoVersion.empty()) {
        printf("Pico Version: %s\n", picoVersion.c_str());
    }

    init_fan_pwm(fd);
    init_watchdog(fd);

cleanup:
    return retVal; 
}
