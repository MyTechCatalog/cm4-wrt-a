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
#include "pico_pkt_fan_pwm.h"

void pkt_fan_pwm(struct pkt_buf *b) {
    uint8_t fan_id = 0;
    bool rw_flag = false; // Read(0)/Write(1) flag
    float pwm_pct = 0.0f;
    bool success = false;
    
    // Unpack the PWM response message
    pico_pkt_fan_pwm_unpack(b->resp, &fan_id, &rw_flag, &pwm_pct, &success);
    
    printf("FAN%d: PWM duty %.1f, R(0)/W(1): %s, Success: %s\n", 
        fan_id, pwm_pct, BOOLEAN_TO_STR(rw_flag), BOOLEAN_TO_STR(success));     
}

void send_fan_pwm_read_request(int fd) {
    uint8_t fan_id = 1;
    bool rw_flag = false; // Read(0)/Write(1) flag
    float pwm_pct = 0.0f;
    pkt_buf pkt = {0,0,0};
    memset((void *)&pkt.req, 0, sizeof(pkt.req));
    memset((void *)&pkt.resp, 0, sizeof(pkt.resp));

    // Pack the PWM request message
    pico_pkt_fan_pwm_req_pack((uint8_t *)pkt.req, fan_id, rw_flag, pwm_pct);

    send_serial_port_command(fd, pkt.req, PICO_PKT_LEN);
}

void send_fan_pwm_write_request(int fd) {
    uint8_t fan_id = 1;
    bool rw_flag = true; // Read(0)/Write(1) flag
    float pwm_pct = appSettings.fan1_pwm;
    pkt_buf pkt = {0,0,0};
    memset((void *)&pkt.req, 0, sizeof(pkt.req));
    memset((void *)&pkt.resp, 0, sizeof(pkt.resp));
    
    // Pack the watchdog request message
    pico_pkt_fan_pwm_req_pack((uint8_t *)pkt.req, fan_id, rw_flag, pwm_pct);

    send_serial_port_command(fd, pkt.req, PICO_PKT_LEN);
}

bool send_fan_pwm_request(int fd, bool rw_flag, uint8_t fan_id, float & pwm_pct) {    
    pkt_buf pkt = {0,0,0};
    memset((void *)&pkt.req, 0, sizeof(pkt.req));
    memset((void *)&pkt.resp, 0, sizeof(pkt.resp));
    
    // Pack the watchdog request message
    pico_pkt_fan_pwm_req_pack((uint8_t *)pkt.req, fan_id, rw_flag, pwm_pct);

    send_serial_port_command(fd, pkt.req, PICO_PKT_LEN);

    bool success = get_pico_response(fd, pkt, PICO_PKT_FAN_PWM_MAGIC);
    
    if (success && !rw_flag) {
        uint8_t fanId = 0;
        bool rwFlag = false;
        // Unpack the PWM response message
        pico_pkt_fan_pwm_unpack(pkt.resp, &fanId, &rwFlag, &pwm_pct, &success);
    }
    
    return success;
}

void init_fan_pwm(int fd) {
    uint8_t fan_id = 1;
    bool rw_flag = true; // Read(0)/Write(1) flag
    float pwm_pct = appSettings.fan1_pwm;
    send_fan_pwm_request(fd, rw_flag, fan_id, pwm_pct);
}