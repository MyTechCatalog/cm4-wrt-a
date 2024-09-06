
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
#include <thread>
#include <csignal>
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
#include "TMP103_I2C.hpp"
#include "PacketHandler.hpp"
#include "fmt/core.h"
#ifdef NO_UBUS
#include "WebServer.hpp"
#else
#include "ubus_server.hpp"
#endif


using namespace picod;

picod::Settings appSettings;

/// @brief Initializes the Raspberry Pi Pico with settings from the 
/// configuration file
/// @return Exit code: 0 - Success, 1 - Failure.
int init_pico();

void quit_signal_handler(int signal) {
    getQuitEvent().set();
}

int main(int argc, char** argv)
{    
    int retVal  = EXIT_SUCCESS;
    std::vector<std::thread> workers;

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

    workers.emplace_back([]() -> void {
        PacketHandler::instance().run();
    });

    // Install a signal handler
    std::signal(SIGINT, quit_signal_handler);
    std::signal(SIGTERM, quit_signal_handler);
    
    if ((retVal = init_pico()) == EXIT_SUCCESS) {
#ifdef NO_UBUS

        if (appSettings.enable_web_interface){
            workers.emplace_back([]() -> void {
                WebServer::instance().start(appSettings.webroot_path);
            });
        }

        WebServer::instance().pico_monitor();

        WebServer::instance().stop();

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        fmt::println("Main thread quitting.");        
#else
        retVal = run_ubus_server();                
#endif
        for (auto& th : workers) th.join();
    }

cleanup:
    return retVal;
}

int init_pico() {    
    int retVal  = EXIT_SUCCESS;
    
    if (PacketHandler::instance().is_init_done()) {
        TMP103_I2C::instance();

        get_pico_version(picoVersion);
        if (!picoVersion.empty()) {
            fmt::println("Pico Version: {}", picoVersion);
        }

        init_fan_pwm();
        init_watchdog();
    } else {
        fmt::println(stderr ,"Init timed out.");
        retVal  = EXIT_FAILURE;
    }

    return retVal; 
}
