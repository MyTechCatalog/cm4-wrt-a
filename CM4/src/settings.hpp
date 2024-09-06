/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PICOD_SETTINGS_H_
#define PICOD_SETTINGS_H_

#include <string>
#include <vector>

#define SANITIZE_PWM_INPUT(pwm)\
pwm = (pwm < 0.0f) ? 0.0f : pwm;\
pwm = (pwm > 1.0f) ? 1.0f : pwm;

namespace picod {
    typedef struct Settings
    {
        /// @brief Specifies how often (in seconds) the host (CM4) 
        /// should poll the Raspberry Pi Pico for temperature readings
        double temperature_poll_interval_seconds;

        /// @brief When set to true, the Raspberry Pi Pico will reset the 
        /// host (CM4) if it does not receive word from picod in the 
        /// number of seconds specified in the setting 
        /// pico_watchdog_timeout_seconds below
        bool enable_watchdog_timer;

        /// @brief If the Raspberry Pi Pico does not recieve a message 
        /// from picod on the host (CM4) in the number of seconds 
        /// specified below, it will reset the CM4 by driving RUN_PG low.
        /// NOTE: If files on a filesystem are open they will not be closed. 
        uint32_t pico_watchdog_timeout_seconds;
       
        /// @brief Pico serial port device path. This is only relevant when
        /// running on a CM4 host
        std::string pico_serial_device_path;

        /// @brief Device path of I2C bus to which the TMP103 temperature sensor is connected. 
        /// This is usually only relevant when running on the CM4 host.
        std::string tmp103_i2c_device_path;

        /// @brief Enables polling of the TMP103 temperature sensor        
        bool enable_tmp103_sensor;

        /// @brief Pulse Width Modulation (PWM) setting for the main system fan (J17)
        float fan1_pwm;

        /// @brief Pulse Width Modulation (PWM) setting for the CM4 fan (J18)
        float cm4_fan_pwm;

        /// @brief Number of times after the first attempt to restart the CM4.
        uint16_t pico_watchdog_max_retries;

        /// @brief Enables publication of sensor data to InfluxDB
        bool enable_influx_db;
        
        /// @brief Name of (InfluxDB) organization
        std::string influx_org_id;
        
        /// @brief InfluxDB API token
        std::string influx_token;
        
        /// @brief Bucket name
        std::string influx_bucket;

        /// @brief InfluxDB host name or IP address
        std::string influx_host;

        /// @brief InfluxDB port number
        uint16_t influx_port;

        /// @brief Names (IDs) of sensors for InfluxDB
        std::vector<std::string> sensorIds;

        /// @brief Length of temperature and fan RPM sensor history in seconds
        uint32_t sensor_history_in_seconds;

        /// @brief HTTP server host name or IP address. 
        std::string http_host;

        /// @brief HTTP server port number
        uint16_t http_port;

        /// @brief Path to local website files.
        std::string webroot_path;
        
        /// @brief Enable(true) HTTP interface (for temperature and fan RPM graphs)
        bool enable_web_interface;

        Settings():
            temperature_poll_interval_seconds(1.0),
            enable_watchdog_timer(false),
            pico_watchdog_timeout_seconds(20),
            pico_serial_device_path("/dev/ttyAMA1"),
            tmp103_i2c_device_path("/dev/i2c-1"),
            enable_tmp103_sensor(true),
            fan1_pwm(0.5f),
            cm4_fan_pwm(0.5f),
            pico_watchdog_max_retries(0),
            enable_influx_db(false),
            influx_host("localhost"),
            influx_port(8086),
            sensorIds({"PCIe_Switch", "M.2_Socket_M_J5", 
                "M.2_Socket_E_J3", "M.2_Socket_M_J2", "RPi_Pico", 
                "System_FAN_J17", "CM4_FAN_J18", "Under_CM4_SOC"}),
            sensor_history_in_seconds(600),
            http_host{"localhost"},
            http_port{8086},
            webroot_path{"/etc/picod/website"},
            enable_web_interface{false} {}

        // Ensure reasonable limits
        void sanitize(){
            const double min_poll_interval = 0.1; // Seconds
            temperature_poll_interval_seconds = 
                (temperature_poll_interval_seconds < min_poll_interval) ?
                min_poll_interval : temperature_poll_interval_seconds;

            SANITIZE_PWM_INPUT(fan1_pwm)
            SANITIZE_PWM_INPUT(cm4_fan_pwm)
            
            pico_watchdog_timeout_seconds = (pico_watchdog_timeout_seconds < 1) ? 
                1 : pico_watchdog_timeout_seconds;
            pico_watchdog_timeout_seconds = (pico_watchdog_timeout_seconds > 0xFFFF) ? 
                0xFFFF : pico_watchdog_timeout_seconds;

            if (influx_host.empty()) {
                influx_host = "localhost";
            }

            if (http_host.empty()) {
                http_host = "localhost";
            }

            sensor_history_in_seconds = (sensor_history_in_seconds < 10) ? 10 : sensor_history_in_seconds;
        }
    } Settings;
}//@END namespace picod

extern picod::Settings appSettings;

#endif