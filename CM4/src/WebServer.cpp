/**
 * Copyright (c) 2024 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */
#include "WebServer.hpp"
#include "version.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <chrono>
#include "fmt/core.h"
//#include "util.h"
#include "NLTemplate.h"
#include "Utils.hpp"
#include "pico_pkt_temperature.h"
#include "pico_pkt_fan_pwm.h"
#include "pico_pkt_watchdog.h"
#include "SensorID.hpp"
#include "version.h"
#include "TMP103_I2C.hpp"
#include "InfluxDB.hpp"

//#include "DataStore.hpp"

#define SET_CONTENT_SUCCESS \
    res.set_content("{\"result\":\"Success\"}", "application/json");

#define SET_CONTENT_INVALID_ARGUMENT(name) \
    { json o; o["error"] = fmt::format("Invalid argument {}", name); res.set_content(o.dump(), "application/json"); }

#define SET_CONTENT_NO_ARGUMENTS \
    res.set_content("{\"error\":\"No arguments provided\"}", "application/json");

#define SET_CONTENT_PICO_CONNECTION_ERROR \
    res.set_content("{\"error\":\"RPI Pico connection error\"}", "application/json");

namespace fs = std::filesystem;
using namespace httplib;
using namespace NL::Template;
using namespace nlohmann;

std::string to_string(const nlohmann::json& j);

namespace picod
{
    std::string picoVersion;
} //@END namespace picod

WebServer::WebServer()
:sequence_number_{0}
{
}

WebServer::~WebServer()
{
}

WebServer & WebServer::instance(){
    static WebServer theInstance;
    return theInstance;
}

void WebServer::stop(){
    if (svr_.is_running()) {
        httplib::Client cli(appSettings.http_host , appSettings.http_port);
        
        if (auto res = cli.Post("/quit/now")) {
            if (res->status != StatusCode::OK_200) {
                auto err = res.error();
                std::cout << "/quit/now HTTP error: " << httplib::to_string(err) << std::endl;
            }
        }
    }
}

void WebServer::update(nlohmann::json message){
    if (!svr_.is_running()) {
        return;
    }

    message["seq"] = sequence_number_++;
    std::stringstream ss;
    ss << "data: " << message.dump() << "\n\n";
    appState_.send_event(ss.str());
}

void WebServer::start(std::string webRoot){
    if (svr_.is_running()) {
        return;
    }

    webRootDir_ = webRoot;

    // Mount / to ./www directory
    auto ret = svr_.set_mount_point("/static", webRootDir_ / "static" );
    if (!ret) {
        fmt::println("Error: The specified base directory {} doesn't exist.", webRootDir_.string());
        return;
    }

    svr_.set_exception_handler([](const auto& req, auto& res, std::exception_ptr ep) {
        auto fmt = "<h1>Error 500</h1><p>%s</p>";
        char buf[BUFSIZ];
        try {
            std::rethrow_exception(ep);
        } catch (std::exception &e) {
            snprintf(buf, sizeof(buf), fmt, e.what());
        } catch (...) { // See the following NOTE
            snprintf(buf, sizeof(buf), fmt, "Unknown Exception");
        }
        res.set_content(buf, "text/html");
        res.status = StatusCode::InternalServerError_500;
    });

    svr_.Get("/stream", [&](const Request & /*req*/, Response &res) {
    
        //std::cout << "/stream: SSE stream opened" << std::endl;

        res.set_chunked_content_provider("text/event-stream",
            [&](size_t /*offset*/, DataSink &sink) {
            appState_.wait_event(&sink);
            return true;
        });
        
        res.set_header("Connection", "keep-alive");
        res.set_header("Cache-Control", "cache, must-revalidate");
    });
    
    svr_.Get("/", [&](const Request& req, Response& res) {        
        json j;
        j["__version__"] = VERSION_STR;

        auto status = get_pico_status();
        j["fan_pwm_pct"] = status["fan_pwm_pct"];
        j["watchdog.timeout_sec"] = status["watchdog"]["timeout_sec"];
        j["watchdog.max_retries"] = status["watchdog"]["max_retries"];       
        
        if (status["watchdog"]["is_enabled"] == true) {
            j["watchdog.is_enabled"] = "checked";
        }

        std::ostringstream os;
        get_template(os, "index", j);    
        
        auto content = os.str();
        char buf[content.size()];
        snprintf(buf, sizeof(buf), content.c_str(), res.status);
        res.set_content(buf, "text/html");
    });
    
    svr_.Post("/quit/now", [&](const Request& req, Response& res) {
        res.set_content("bye", "text/plain");
        svr_.stop();
        getQuitEvent().set();
    });

    svr_.Post("/api/start", [&](const Request& req, Response& res) {
        appState_.resume();
        SET_CONTENT_SUCCESS
    });

    svr_.Post("/api/stop", [&](const Request& req, Response& res) {
        //std::cout << "/api/stop" << std::endl;
        appState_.pause();
        SET_CONTENT_SUCCESS
    });

    svr_.Get("/api/status", [&](const Request& req, Response& res) {
        //std::cout << "/api/status" << std::endl;
        auto status = get_pico_status();
        res.set_content(status.dump(), "application/json");
    });

    svr_.Post("/api/settings/:name", [&](const Request& req, Response& res) {
        auto name = req.path_params.at("name");

        auto constexpr is_valid_setting_name = [](auto const & aName) {
            auto static const names = std::set<std::string>{"watchdog", "fan_pwm"};
            return !!names.count(aName);
        };

        if(!is_valid_setting_name(name)){
            res.set_content(fmt::format("{\"error\":\"Setting not found: {}\"}", name), "application/json");
            return;
        }

        //fmt::println("/api/settings: {}", req.body);

        if (name == "fan_pwm") {
            if (req.body.empty()) {
                SET_CONTENT_NO_ARGUMENTS
                return;
            }

            json d;
            
            try {
                d = json::parse(req.body);

                if (d.empty()) {
                    SET_CONTENT_NO_ARGUMENTS
                    return;
                }

                auto key = "fan_name";
                if (!d.contains(key) || d[key].empty() || !d[key].is_string()) {
                    SET_CONTENT_INVALID_ARGUMENT(key);
                    return;
                }                
                
                json result;

                auto fan_name = d[key].get<std::string>();
                uint8_t fan_id = get_fan_id_from_name(fan_name.c_str());

                if (fan_id ==  INVALID_FAN_ID) {
                    SET_CONTENT_INVALID_ARGUMENT(fan_name);
                    return;
                }

                key = "fan_pwm_pct";
                if (d.contains(key) && !d[key].is_number_unsigned()) {
                    SET_CONTENT_INVALID_ARGUMENT(key);
                    return;
                }

                uint32_t fan_pwm_pct = 0;
                bool rw_flag = false;

                if (d.contains(key)) {
                    rw_flag = true;
                    fan_pwm_pct = d[key].get<uint32_t>();
                }
                
                fan_pwm_pct = fan_pwm_pct > 100 ? 100 : fan_pwm_pct;
                float fan_pwm =  static_cast<float>(fan_pwm_pct)/FAN_PWM_LSB;

                std::vector<struct pico_pkt_fan_pwm_t> fanInfo;
                fanInfo.emplace_back(pico_pkt_fan_pwm_t { 
                        .fan_id = static_cast<uint8_t>(fan_id), .pwm_pct = fan_pwm });

                if (send_fan_pwm_request(rw_flag, fanInfo)) {                        
                    result[fan_name] = static_cast<uint32_t>(fanInfo[0].pwm_pct*100);
                } else {
                    SET_CONTENT_PICO_CONNECTION_ERROR
                    return;
                }

                res.set_content(result.dump(), "application/json");

            } catch (std::exception &e) {
                //fmt::println("Error: {}", e.what());
                res.set_content("{\"error\":\"Invalid JSON object.\"}", "application/json");
                res.status = StatusCode::InternalServerError_500;
            }
        } else if (name == "watchdog") {
            if (req.body.empty()) {
                SET_CONTENT_NO_ARGUMENTS
                return;
            }

            json d;
            
            try {
                d = json::parse(req.body);

                if (d.empty()) {
                    SET_CONTENT_NO_ARGUMENTS
                    return;
                }
                
                pico_pkt_watchdog_t s = {0};
                auto key = "is_enabled";
                if (!d.contains(key) || !d[key].is_boolean()) {
                    SET_CONTENT_INVALID_ARGUMENT(key);
                    return;
                }
                s.enable = d[key].get<bool>();
        
                key = "timeout_sec";
                if (!d.contains(key) || !d[key].is_number_unsigned()) {
                    SET_CONTENT_INVALID_ARGUMENT(key);
                    return;
                }
                s.timeout = d[key].get<uint16_t>();
                s.timeout = sanitize_watchdog_timeout(s.timeout);

                key = "max_retries";
                if (!d.contains(key) || !d[key].is_number_unsigned()) {
                    SET_CONTENT_INVALID_ARGUMENT(key);
                    return;
                }
                s.max_retries = d[key].get<uint16_t>();

                //fmt::println("/api/settings: sanitize_watchdog_timeout");
                s.write = true;

                if (send_watchdog_request(s)) {
                    json result;
                    result["is_enabled"] = s.enable;
                    result["timeout_sec"] = s.timeout;
                    result["max_retries"] = s.max_retries;
                    res.set_content(result.dump(), "application/json");
                } else {
                    SET_CONTENT_PICO_CONNECTION_ERROR
                    return;
                }
            } catch (std::exception &e) {
                //fmt::println("Error: {}", e.what());
                res.set_content("{\"error\":\"Invalid JSON object.\"}", "application/json");
                res.status = StatusCode::InternalServerError_500;
            }         
        }     
    });

    /*svr.set_logger([](const Request &req, const Response &res) {
        printf("%s", log(req, res).c_str());
    });*/

    fmt::println("Serving at http://{}:{}/", appSettings.http_host, appSettings.http_port);
    svr_.listen(appSettings.http_host , appSettings.http_port);        
}

nlohmann::json WebServer::get_pico_status() {
    bool rw_flag = false; // Read(0)/Write(1) flag
    json status;    
    
    std::vector<struct pico_pkt_fan_pwm_t> fanInfo;
    fanInfo.emplace_back(pico_pkt_fan_pwm_t { 
        .fan_id = SYS_FAN1, .pwm_pct = 0.0f });
    fanInfo.emplace_back(pico_pkt_fan_pwm_t { 
        .fan_id = CM4_FAN, .pwm_pct = 0.0f });

    if (send_fan_pwm_request(rw_flag, fanInfo)) {
        json j;
        j[appSettings.sensorIds[picod::System_FAN_J17]] = fanInfo[SYS_FAN1-1].pwm_pct*100.0;
        j[appSettings.sensorIds[picod::CM4_FAN_J18]]    = fanInfo[CM4_FAN-1].pwm_pct*100.0;
        status["fan_pwm_pct"] = j;
    }

    pico_pkt_watchdog_t s = {0};
    s.write = rw_flag; 
    if (send_watchdog_request(s)) {
        json j;
        j["is_enabled"] = s.enable;
        j["timeout_sec"] = s.timeout;
        j["max_retries"] = s.max_retries;
        status["watchdog"] = j;
    }

    pico_pkt_temperature_u t = {0};
    if (send_temperature_request(t)) {
        json j, j2;
        if (appSettings.sensorIds.size() > NUM_NTC_SENSORS) {
            for (int ch=0; ch < NUM_NTC_SENSORS; ch++) {
                j[appSettings.sensorIds[ch]] = t.data[ch];
            }
        }

        if (picod::SensorId::NUM_SENSOR_IDs == appSettings.sensorIds.size()) {
            j[appSettings.sensorIds[picod::RPi_Pico]] = t.s.pico;

            if (appSettings.enable_tmp103_sensor) { 
                auto retVal = picod::TMP103_I2C::instance().getTemperature();
                j[appSettings.sensorIds[picod::Under_CM4_SOC]] = retVal;
            }

            j2[appSettings.sensorIds[picod::System_FAN_J17]] = t.s.fan1rpm;
            j2[appSettings.sensorIds[picod::CM4_FAN_J18]]    = t.s.cm4_fan_rpm;            
        }

        status["temperature_c"] = j;
        status["tachometer_rpm"] = j2;
    }

    return status;
}

void WebServer::saveTemperatureSensorReading(const std::string & name, 
    const  float & value){
    if (auto search = tempDict_.find(name); search != tempDict_.end()) {
        if (search->second->size() == appSettings.sensor_history_in_seconds){
            search->second->pop_front();
        }                    
        search->second->push_back(value);
    } else {
        auto item = std::pair{name, std::make_shared<std::deque<float>>()};
        tempDict_.insert(item);
        item.second->push_back(value);
    }
    
}

void WebServer::saveFanRpmSensorReading(const std::string & name, 
    const uint32_t & value){
    if (auto search = rpmDict_.find(name); search != rpmDict_.end()) {
        if (search->second->size() == appSettings.sensor_history_in_seconds){
            search->second->pop_front();
        }                    
        search->second->push_back(value);
    } else {
        auto item = std::pair{name, std::make_shared<std::deque<uint32_t>>()};
        rpmDict_.insert(item);
        item.second->push_back(value);
    }
}

void WebServer::pico_monitor() {
    pico_pkt_temperature_u t = {0};

    while (!getQuitEvent().wait(std::chrono::milliseconds(
        static_cast<int64_t>(appSettings.temperature_poll_interval_seconds*1000.0)))) {

        if (!send_temperature_request(t)) {
            continue;
        }
        
        if (appSettings.sensorIds.size() > NUM_NTC_SENSORS) {
            for (int ch=0; ch < NUM_NTC_SENSORS; ch++) {
                saveTemperatureSensorReading(appSettings.sensorIds[ch], t.data[ch]);
                
                if (appSettings.enable_influx_db) {
                    picod::InfluxDB::instance().addTemperature(
                        appSettings.sensorIds[ch], t.data[ch]);
                }
            }
        }

        if (picod::SensorId::NUM_SENSOR_IDs == appSettings.sensorIds.size()) {
            saveTemperatureSensorReading(appSettings.sensorIds[picod::RPi_Pico], t.s.pico);
            
            if (appSettings.enable_influx_db) {
                picod::InfluxDB::instance().addTemperature(
                    appSettings.sensorIds[picod::RPi_Pico], t.s.pico);

                picod::InfluxDB::instance().addTachometer(
                    appSettings.sensorIds[picod::System_FAN_J17], t.s.fan1rpm);
            }

            if (appSettings.enable_tmp103_sensor) {
                float retVal = picod::TMP103_I2C::instance().getTemperature();

                saveTemperatureSensorReading(appSettings.sensorIds[picod::Under_CM4_SOC], retVal);

                if (appSettings.enable_influx_db) {
                    picod::InfluxDB::instance().addTemperature(
                    appSettings.sensorIds[picod::Under_CM4_SOC], retVal);
                }
            }

            saveFanRpmSensorReading(appSettings.sensorIds[picod::System_FAN_J17], t.s.fan1rpm);
            saveFanRpmSensorReading(appSettings.sensorIds[picod::CM4_FAN_J18], t.s.cm4_fan_rpm);
            
            const auto p1 = std::chrono::system_clock::now();
            auto t = std::chrono::duration_cast<std::chrono::seconds>(
                   p1.time_since_epoch()).count();
            
            if (timeStamps_.size() == appSettings.sensor_history_in_seconds) {
                timeStamps_.pop_front();
            }

            timeStamps_.push_back(t);

            if (appSettings.enable_web_interface) {
                json j;
                j["temperature_c"] = json::object();
                j["tachometer_rpm"] = json::object();
                j["timestamp_sec"] = timeStamps_;

                for (auto const& [key, val] : tempDict_){
                    j["temperature_c"][key] = *val.get();
                }

                for (auto const& [key, val] : rpmDict_){
                    j["tachometer_rpm"][key] = *val.get();
                }
                            
                WebServer::instance().update(j);
            }

            if (appSettings.enable_influx_db) {
                picod::InfluxDB::instance().publish();
            }
        }
    }
}

std::string to_string(const nlohmann::json& j)
{
    switch (j.type())
    {
        // avoid escaping string value
        case json::value_t::string:
            return j.get<std::string>();

        /*case json::value_t::array:
        case json::value_t::object:
            return "";*/

        // use dump() for all other value types
        default:
            return j.dump();
    }
}

void WebServer::get_template(std::ostream & content, std::string name, nlohmann::json & vars, bool show_navbar){
    NL::Template::LoaderMemory loader;    
    NL::Template::Template layout( loader );

    fs::path filePath(webRootDir_ / "templates" / (name + ".html"));
    if (!fs::is_regular_file(filePath)) {
        fmt::println("Error, file not found: {}", filePath.string());
        return;
    }       
        
    loader.add( "base", read_file(filePath.string()) );    
    layout.load( "base" );

    Block & contentBlock = layout.block( "content" );

    for (auto& [key, value] : vars.items()) {
        try {
            contentBlock.set(key, to_string(value));            
        } catch (std::exception &e) {
            std::cout << __FILE__ << ":" << __LINE__ << ":" << __func__ << "()" << e.what() << std::endl;
        }
    }

    layout.render( content ); // Render the template with the variables we've set above 
}

std::string WebServer::dump_headers(const Headers &headers) {
  std::string s;
  char buf[BUFSIZ];

  for (auto it = headers.begin(); it != headers.end(); ++it) {
    const auto &x = *it;
    snprintf(buf, sizeof(buf), "%s: %s\n", x.first.c_str(), x.second.c_str());
    s += buf;
  }

  return s;
}

std::string WebServer::log(const Request &req, const Response &res) {
    std::string s;
    char buf[BUFSIZ];

    s += "================================\n";

    snprintf(buf, sizeof(buf), "%s %s %s", req.method.c_str(),
            req.version.c_str(), req.path.c_str());
    s += buf;

    std::string query;
    for (auto it = req.params.begin(); it != req.params.end(); ++it) {
        const auto &x = *it;
        snprintf(buf, sizeof(buf), "%c%s=%s",
                (it == req.params.begin()) ? '?' : '&', x.first.c_str(),
                x.second.c_str());
        query += buf;
    }
    snprintf(buf, sizeof(buf), "%s\n", query.c_str());
    s += buf;

    s += dump_headers(req.headers);

    s += "--------------------------------\n";

    snprintf(buf, sizeof(buf), "%d %s\n", res.status, res.version.c_str());
    s += buf;
    s += dump_headers(res.headers);
    s += "\n";

    if (!res.body.empty()) { s += res.body; }

    s += "\n";

    return s;
}