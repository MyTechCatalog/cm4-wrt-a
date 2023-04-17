/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */
#include "InfluxDB.hpp"
#include "settings.hpp"
#include <iostream>

namespace picod {

InfluxDB::InfluxDB(std::string hostname, std::string org_id, 
    std::string token, std::string bucket, u_int16_t port):
hostname_(hostname),
org_id_(org_id),
apiToken_(token),
bucket_(bucket),
portNum_(port),
dataOut_(std::ios_base::out | std::ios_base::binary),
client_(geWriteEndpointURL()) {    
    client_.setHeader("Authorization: Token " + token);
    client_.setHeader("Content-Type: text/plain; charset=utf-8");
    client_.setHeader("Accept: application/json");
}

InfluxDB::~InfluxDB()
{
}

InfluxDB& InfluxDB::instance(){
    static InfluxDB inst(appSettings.influx_host,
        appSettings.influx_org_id, appSettings.influx_token,
        appSettings.influx_bucket, appSettings.influx_port);
    return inst;
}

std::string InfluxDB::geWriteEndpointURL(){
    std::ostringstream os;
    os << "http://"<< hostname_ <<":"<< portNum_ <<"/api/v2/write?org="<<
    org_id_ << "&bucket=" << bucket_ << "&precision=ms";
    //std::cout << "InfluxDB URL: " << os.str() << std::endl;
    return os.str();
}

void InfluxDB::publish() {
    // If the stream is not empty:
    if ((dataOut_.tellp() != 0)) {
        std::string rcvd_message;
        client_.postRequest(dataOut_.str(),rcvd_message, lastError_);
        if (!rcvd_message.empty()){
            std::cout << rcvd_message << std::endl;
        }     
        dataOut_.str("");
        dataOut_.clear();   
    }
}

void InfluxDB::addTemperature(std::string sensorId, float value){
    //TemperatureSensors,sensor_id=NTC1 temperature=33.45
    dataOut_ << "TemperatureSensors,sensor_id=" << sensorId <<
    " temperature=" << value << std::endl;    
}

void InfluxDB::addTachometer(std::string sensorId, float value){
    //FanTachometers,sensor_id=FAN1 rpm=4800
    dataOut_ << "FanTachometers,sensor_id=" << sensorId <<
    " rpm=" << value << std::endl;
}

} //@END namespace picod