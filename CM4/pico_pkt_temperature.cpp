/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <cerrno>
#include <cstring> // memset
#include <unistd.h>
#include <iostream>
#include "Utils.hpp"
#include "InfluxDB.hpp"
#include "pico_pkt_temperature.h"
#include "SensorID.hpp"

void pkt_temperature(struct pkt_buf *b)
{
    pico_pkt_temperature_u tmp;
    bool success = true;

    pico_pkt_temperature_resp_unpack((uint8_t *)b->resp, &tmp, &success);

    if (appSettings.enable_influx_db) {
        if (appSettings.sensorIds.size() > NUM_NTC_SENSORS) {
            for (int ch=0; ch < NUM_NTC_SENSORS; ch++) {
                picod::InfluxDB::instance().addTemperature(
                    appSettings.sensorIds[ch], tmp.data[ch]);
                //printf("NTC%d: %.1f°C\n", (ch+1), tmp.data[ch]);
            }
        }

        if (picod::SensorId::NUM_SENSOR_IDs == appSettings.sensorIds.size()) {
            picod::InfluxDB::instance().addTemperature(
                appSettings.sensorIds[picod::RPi_Pico], tmp.s.pico);

            picod::InfluxDB::instance().addTachometer(
                appSettings.sensorIds[picod::System_FAN_J17], tmp.s.fan1rpm);
        }
    }//@END if (appSettings.enable_influx_db)

    //printf("Pico: %.1f°C\n", tmp.s.pico);
    //printf("FAN1: %d RPM\n--------\n", tmp.s.fan1rpm);
}


void send_temperature_request(int fd){
    pkt_buf pkt = {0,0,0};

    memset((void *)&pkt.req, 0, sizeof(pkt.req));
    memset((void *)&pkt.resp, 0, sizeof(pkt.resp));

    pico_pkt_temperature_req_pack((uint8_t *)pkt.req);

    send_serial_port_command(fd, pkt.req, PICO_PKT_LEN);
}

bool send_temperature_request(int fd, pico_pkt_temperature_u & tmp) {
    bool success = true;

    pkt_buf pkt = {0,0,0};

    memset((void *)&pkt.req, 0, sizeof(pkt.req));
    memset((void *)&pkt.resp, 0, sizeof(pkt.resp));

    pico_pkt_temperature_req_pack((uint8_t *)pkt.req);

    send_serial_port_command(fd, pkt.req, PICO_PKT_LEN);
    
    success = get_pico_response(fd, pkt, PICO_PKT_TEMPERATURE_MAGIC);
    
    if (success) {        
        // Unpack the temperature response message
        pico_pkt_temperature_resp_unpack((uint8_t *)pkt.resp, &tmp, &success);
    }
    
    return success;    
}