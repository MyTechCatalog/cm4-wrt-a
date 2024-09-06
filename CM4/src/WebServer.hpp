/**
 * Copyright (c) 2024 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef WEB_SERVER_HPP
#define WEB_SERVER_HPP
#include "httplib.h"
#include <filesystem>
#include "SSEDispatcher.hpp"
#include "json.hpp"
#include <map>
#include <deque>

namespace picod {
    extern std::string picoVersion;
} //@END namespace picod

class WebServer
{
public:    
    static WebServer & instance();
    ~WebServer();
    WebServer(WebServer const&) = delete;
    void operator=(WebServer const&) = delete;

    void start(std::string webRoot);
    void stop();
    void update(nlohmann::json message);
    void pico_monitor();
    nlohmann::json get_pico_status();

private:
    httplib::Server svr_;
    SSEDispatcher appState_;
    size_t sequence_number_;
    std::filesystem::path webRootDir_;
    /// @brief Stores temperature history
    std::map<std::string, std::shared_ptr<std::deque<float>>> tempDict_;
    /// @brief Stores FAN RPM history
    std::map<std::string, std::shared_ptr<std::deque<uint32_t>>> rpmDict_;
    std::deque<size_t> timeStamps_; 

    WebServer();
    static std::string log(const httplib::Request &req, const httplib::Response &res);
    static std::string dump_headers(const httplib::Headers &headers);
    void saveTemperatureSensorReading(const std::string & name, const float & value);
    void saveFanRpmSensorReading(const std::string & name, const uint32_t & value);

    /// @brief Renders HTML template given its name and variables.
    /// @param content Rendered output.
    /// @param name Template name e.g home.html.
    /// @param vars JSON object containing template variables.
    /// @param show_navbar Show the nav bar when true.
    void get_template(std::ostream & content, std::string name, nlohmann::json & vars, bool show_navbar = true);
};

#endif