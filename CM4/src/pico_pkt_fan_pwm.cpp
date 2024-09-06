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
#include <mutex>
#include "Utils.hpp"
#include "pico_pkt_fan_pwm.h"
#include "SensorID.hpp"
#include "PacketHandler.hpp"

void pkt_fan_pwm(struct pkt_buf *b) {    
    bool rw_flag = false; // Read(0)/Write(1) flag    
    bool success = false;
    struct pico_pkt_fan_pwm_t fans[NUM_PWM_FANS] = {
        { .fan_id = SYS_FAN1, .pwm_pct = 0.0f },
        { .fan_id = CM4_FAN, .pwm_pct = 0.0f },
    };
    
    // Unpack the PWM response message
    pico_pkt_fan_pwm_unpack(b->resp, fans, &rw_flag, &success);

    for (size_t i = 0; i < NUM_PWM_FANS; i++) {
        printf("FAN%d: PWM duty %.1f\n", 
        fans[i].fan_id, fans[i].pwm_pct);
    }

    printf("R(0)/W(1): %s, Success: %s\n", 
        BOOLEAN_TO_STR(rw_flag), BOOLEAN_TO_STR(success));     
}

bool send_fan_pwm_request(bool rw_flag, 
    std::vector<struct pico_pkt_fan_pwm_t> &fanInfo) {

    pkt_buf pkt = {0,0,0};
    memset((void *)&pkt.req, 0, sizeof(pkt.req));
    memset((void *)&pkt.resp, 0, sizeof(pkt.resp));
    struct pico_pkt_fan_pwm_t fans[NUM_PWM_FANS] = {
        { .fan_id = INVALID_FAN_ID, .pwm_pct = 0.0f },
        { .fan_id = INVALID_FAN_ID, .pwm_pct = 0.0f }
    };
    
    {
        const size_t TIMES_TO_LOOP = (fanInfo.size() > NUM_PWM_FANS) ? NUM_PWM_FANS : fanInfo.size();
        
        for (size_t i = 0; i < TIMES_TO_LOOP; i++) {
            if (!is_valid_pico_fan_id(fanInfo[i].fan_id) || (fanInfo[i].fan_id < 1)) {
                continue;
            }
            
            uint8_t fan_idx = (fanInfo[i].fan_id - 1);
            
            fans[fan_idx].fan_id = fanInfo[i].fan_id;
            fans[fan_idx].pwm_pct = fanInfo[i].pwm_pct;
        }       
    }
    
    // Pack the PWM request message
    pico_pkt_fan_pwm_req_pack((uint8_t *)pkt.req, fans, rw_flag);    

    PacketHandler::instance().send_pico_request(pkt.req, PICO_PKT_LEN);

    bool success = PacketHandler::instance().get_pico_response(pkt, PICO_PKT_FAN_PWM_MAGIC);
    
    bool local_rw_Flag = rw_flag;

    if (success) {        
        // Unpack the PWM response message
        pico_pkt_fan_pwm_unpack(pkt.resp, fans, &local_rw_Flag, &success);

        success = success && (local_rw_Flag == rw_flag);

        const size_t TIMES_TO_LOOP = (fanInfo.size() > NUM_PWM_FANS) ? NUM_PWM_FANS : fanInfo.size();
        for (size_t i = 0; i < TIMES_TO_LOOP; i++) {
            if (!is_valid_pico_fan_id(fanInfo[i].fan_id) || (fanInfo[i].fan_id < 1)) {
                continue;
            }
            
            uint8_t fan_idx = (fanInfo[i].fan_id - 1);

            if (fanInfo[i].fan_id == fans[fan_idx].fan_id) {
                fanInfo[i].pwm_pct = fans[fan_idx].pwm_pct;
            }
        }
    }
    
    return success;
}

void init_fan_pwm() {    
    bool rw_flag = true; // Read(0)/Write(1) flag    
    std::vector<struct pico_pkt_fan_pwm_t> fanInfo;

    fanInfo.emplace_back(pico_pkt_fan_pwm_t { 
        .fan_id = SYS_FAN1, .pwm_pct = appSettings.fan1_pwm });

    fanInfo.emplace_back(pico_pkt_fan_pwm_t { 
        .fan_id = CM4_FAN, .pwm_pct = appSettings.cm4_fan_pwm });

    send_fan_pwm_request(rw_flag, fanInfo);
}

void turn_off_fan_pwm() {    
    bool rw_flag = true; // Read(0)/Write(1) flag    
    std::vector<struct pico_pkt_fan_pwm_t> fanInfo;

    fanInfo.emplace_back(pico_pkt_fan_pwm_t { 
        .fan_id = SYS_FAN1, .pwm_pct = 0.0f });

    fanInfo.emplace_back(pico_pkt_fan_pwm_t { 
        .fan_id = CM4_FAN, .pwm_pct = 0.0f });

    send_fan_pwm_request(rw_flag, fanInfo);
}

uint8_t get_fan_id_from_name(const char * fan_name) {
    uint8_t retVal = INVALID_FAN_ID;

    if ( strcasecmp(fan_name,appSettings.sensorIds[picod::System_FAN_J17].c_str()) == 0 ) {
        retVal = SYS_FAN1;
    } else if ( strcasecmp(fan_name,appSettings.sensorIds[picod::CM4_FAN_J18].c_str()) == 0 ) {
        retVal = CM4_FAN;
    }

    return retVal;   
}