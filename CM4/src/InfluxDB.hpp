/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef INFLUX_DB_HPP_
#define INFLUX_DB_HPP_
#include <sstream>
#include "httplib.h"

namespace picod {
/// @brief This is a helper class used to publish sensor
/// data to InfluxDB
class InfluxDB {
public:
    static InfluxDB& instance();   
    ~InfluxDB();
    InfluxDB(InfluxDB const&)     = delete;
    void operator=(InfluxDB const&)  = delete;   
    
    /// @brief Sends any aggregated sensor data to InfluxDB
    void publish();

    /// @brief Aggregates temperature sensor data
    /// @param sensorId Sensor ID (name)
    /// @param value Temperature (°C)
    void addTemperature(std::string sensorId, float value);

    /// @brief Aggregates tachometer sensor data
    /// @param sensorId  Sensor ID (name)
    /// @param value Revolutions Per Minute (RPM)
    void addTachometer(std::string sensorId, float value);
private:    
    /// @brief InfluxDB host, e.g. localhost
    std::string hostname_;
    /// @brief Organization
    std::string org_id_;
    /// @brief API token
    std::string apiToken_;
    /// @brief Bucket name
    std::string bucket_;
    /// @brief InfluxDB port number
    u_int16_t portNum_;
    /// @brief Endpoint path
    std::string path_;
    
    /// @brief Outgoing sensor data is assembles using this object
    std::ostringstream dataOut_;

    /// @brief httplib helper object
    httplib::Client client_;

    InfluxDB(std::string hostname, std::string org_id, 
    std::string token, std::string bucket, u_int16_t port);

    /// @brief Generates the URL used write data to 
    /// InfluxDB using an HTTP request 
    /// @return The InfluxDB API /api/v2/write endpoint URL
    std::string geWriteEndpointURL();
};

} //@END namespace picod

#endif //@END INFLUX_DB_HPP_