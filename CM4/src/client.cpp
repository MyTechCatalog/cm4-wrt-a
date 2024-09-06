/**
 * Copyright (c) 2024 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <string>
#include <iostream>
//#include <memory>
#include "json.hpp"
#include "cxxopts.hpp"
#include "httplib.h"
#include "fmt/core.h"

int main(int argc, char const *argv[])
{
    try
    {
        std::unique_ptr<cxxopts::Options> allocated(new cxxopts::Options(argv[0], "Picod commandline client"));
        auto& options = *allocated;

        options
        .set_width(70)
        .set_tab_expansion()
        .allow_unrecognised_options()
        .add_options()
        ("f,fan", "Fan Name: Specifies the name of a fan (CM4_FAN_J18, System_Fan_J17).", cxxopts::value<std::string>())
        ("p,pwm", "Fan's Pulse Width Modulation (PWM) duty cycle as a percentage [0 to 100]", cxxopts::value<uint32_t>()->default_value("50"))
        ("w,watchdog", "Enable(1)/Disable(0) the watchdog feature.", cxxopts::value<uint16_t>()->default_value("0"))
        ("t,timeout", "Watchdog timeout value in seconds.", cxxopts::value<uint16_t>()->default_value("10"))
        ("m,max-retries", "The maximum number of times the watchdog (RPi Pico) will attempt to restart the CM4.", cxxopts::value<uint16_t>()->default_value("0"))
        ("s,status", "Get status.", cxxopts::value<bool>()->default_value("false"))
        ("n,port", "HTTP server port number", cxxopts::value<uint16_t>()->default_value("8086"))
        ("h,help", "Print help")
        ;

        auto result = options.parse(argc, argv);
        
        if (result.count("help") || (argc == 1)) {
            std::cout << options.help() << std::endl;
            return false;
        }
        
        auto port = result["port"].as<uint16_t>();

        if (result.count("status")) {
             httplib::Client cli("localhost", port);

            if (auto res = cli.Get("/api/status")) {
                if (res->status == httplib::StatusCode::OK_200) {
                    try {
                        using namespace nlohmann;
                        json d = json::parse(res->body);
                        fmt::println("picod response:\n{}", d.dump(4));
                    } catch (std::exception &e) {
                        fmt::println("Error: {}", e.what());
                    }
                }
            } else {
                auto err = res.error();
                std::cout << "HTTP error: " << httplib::to_string(err) << std::endl;
            }
        }
        
        if (result.count("fan")) {
            using namespace nlohmann;
            json j;
            auto fan_name = result["fan"].as<std::string>();
            j["fan_name"] = fan_name;

            if (result.count("pwm")) {
                auto fan_pwm = result["pwm"].as<uint32_t>();
                j["fan_pwm_pct"] = fan_pwm;
            }

            httplib::Client cli("localhost", port);

            if (auto res = cli.Post("/api/settings/fan_pwm", j.dump(), "application/json")) {
                if (res->status == httplib::StatusCode::OK_200) {
                    try {
                        json d = json::parse(res->body);
                        fmt::println("picod response:\n{}", d.dump(4));
                    } catch (std::exception &e) {
                        fmt::println("Error: {}", e.what());
                    }
                }
            } else {
                auto err = res.error();
                fmt::println("HTTP error: {}", httplib::to_string(err));
            }
        }

        if (result.count("watchdog")) {
            using namespace nlohmann;
            json j;
            auto is_enabled = !!result["watchdog"].as<uint16_t>();             
            auto timeout_sec = result["timeout"].as<uint16_t>();
            auto max_retries = result["max-retries"].as<uint16_t>();

            j["is_enabled"] = is_enabled;
            j["timeout_sec"] = timeout_sec;
            j["max_retries"] = max_retries;
            
            httplib::Client cli("localhost", port);

            if (auto res = cli.Post("/api/settings/watchdog", j.dump(), "application/json")) {
                if (res->status == httplib::StatusCode::OK_200) {
                    try {
                        json d = json::parse(res->body);
                        fmt::println("picod response:\n{}", d.dump(4));
                    } catch (std::exception &e) {
                        fmt::println("Error: {}", e.what());
                    }
                }
            } else {
                auto err = res.error();
                fmt::println("HTTP error: {}", httplib::to_string(err));
            }
        }
    } catch (const cxxopts::exceptions::exception& e) {
        fmt::println("error parsing options: {}", e.what());
        return false;
    }

    return 0;
}
