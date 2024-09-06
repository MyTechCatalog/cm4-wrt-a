/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */
#include "syshead.h"
#include "cli.hpp"
#include "Utils.hpp"
#include "version.h"
#include <libconfig.h>
#include <filesystem>
#include "settings.hpp"
#include "fmt/core.h"

namespace fs = std::filesystem;

void usage(char *app) {
    print_err("Raspberry Pi Pico Monitoring Service.\nVersion %s\n",VERSION_STR);
    print_err("Monitors the RPi Pico on CM4-WRT-A router board.\n\n");
    print_err("Usage: %s %s\n", app, "-c \"/path/to/config_file\"\n");
    print_err( "  [-h] Print usage.\n");
    print_err( "  [-v] Print version.\n");
    print_err("\n");
    exit(1);
}

void print_version() {
    printf("Version %s\n",VERSION_STR);
}

extern int optind;

static int parse_opts(int *argc, char*** argv)
{
    int opt;
    
    while ((opt = getopt(*argc, *argv, "c:hv")) != -1) {
        switch (opt) {              
        case 'c':
            set_config_file_path(optarg);            
            break;
        case 'v':
            print_version();
            exit(0);
            break;
        case 'h':
        default:
            usage(*argv[0]);
        }
    }

    *argc -= optind;
    *argv += optind;

    return optind;
}

void parse_cli(int argc, char **argv)
{
    if (argc == 1) {
        usage(argv[0]);
    } else {
        parse_opts(&argc, &argv);
    }    
}

int parse_config_file(const char * file_path) 
{
    int ret_val = 0;    
    config_t config;
       
    config_init (&config);

    if( access( file_path, F_OK ) != 0 ) {
        fmt::println(stderr,"Config file not found: {}", file_path);
        ret_val = EXIT_FAILURE;
        goto clean_up;
    }
   

    if (!config_read_file (&config, file_path)) {
        fmt::println(stderr,"{}: {}", config_error_text(&config), config_error_file(&config));
        fmt::println(stderr, "Error reading file: {}", file_path);
        ret_val = EXIT_FAILURE;
        goto clean_up;
    } 
    
    GET_FLOAT_SETTING("temperature_poll_interval_seconds", appSettings.temperature_poll_interval_seconds)
    GET_BOOLEAN_SETTING("enable_watchdog_timer", appSettings.enable_watchdog_timer)
    GET_INTEGER32_SETTING("pico_watchdog_timeout_seconds", appSettings.pico_watchdog_timeout_seconds)
    GET_STRING_SETTING("pico_serial_device_path", appSettings.pico_serial_device_path)
    GET_STRING_SETTING("tmp103_i2c_device_path", appSettings.tmp103_i2c_device_path)
    GET_BOOLEAN_SETTING("enable_tmp103_sensor", appSettings.enable_tmp103_sensor)
    GET_FLOAT_SETTING("fan1_pwm", appSettings.fan1_pwm)
    GET_FLOAT_SETTING("cm4_fan_pwm", appSettings.cm4_fan_pwm)
    GET_INTEGER16_SETTING("pico_watchdog_max_retries", appSettings.pico_watchdog_max_retries)
    GET_BOOLEAN_SETTING("enable_influx_db", appSettings.enable_influx_db)
    GET_STRING_SETTING("influx_host", appSettings.influx_host)
    GET_STRING_SETTING("influx_org_id", appSettings.influx_org_id)
    GET_STRING_SETTING("influx_bucket", appSettings.influx_bucket)
    GET_STRING_SETTING("influx_token", appSettings.influx_token)
    GET_INTEGER32_SETTING("sensor_history_in_seconds", appSettings.sensor_history_in_seconds)
    GET_STRING_SETTING("http_host", appSettings.http_host)
    GET_INTEGER16_SETTING("http_port", appSettings.http_port)
    GET_STRING_SETTING("webroot_path", appSettings.webroot_path)
    GET_BOOLEAN_SETTING("enable_web_interface", appSettings.enable_web_interface)

    if (!fs::is_directory(appSettings.webroot_path)) {
        fmt::println(stderr, "Error, webroot directory not found: {}", appSettings.webroot_path);
    }    

    { // Parse the InfluxDB sensor IDs
        config_setting_t *setting = config_lookup(&config, "sensor_names");
        if(setting != NULL) {
            size_t count = config_setting_length(setting);
            const size_t NUM_SENSORS = appSettings.sensorIds.size();
            if (count == NUM_SENSORS) {
                for (size_t i = 0; i < count; i++) {
                    const char *name = config_setting_get_string_elem(setting, i);
                    if (name && (strlen(name) > 1)) {
                        appSettings.sensorIds[i] = name;
                    }
                }
            } else {
                fmt::println(stderr, "Error reading file: {}\nError {} sensor IDs expected in setting \"{}}\"", 
                    file_path, NUM_SENSORS, setting->name);
                ret_val = EXIT_FAILURE;
            }
        }
    }
    
    appSettings.sanitize();

clean_up:
   config_destroy (&config);   
   return ret_val;
}