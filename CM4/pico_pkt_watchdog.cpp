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
#include "pico_pkt_watchdog.h"

void pkt_watchdog(struct pkt_buf *b) {    
    pico_pkt_watchdog_t s = {0};
    
    // Unpack the watchdog response message
    pico_pkt_watchdog_unpack(b->resp, &s);
    printf("Watchdog Timeout: %d (Sec), Retries: %d, "\
        "Enabled: %s, R(0)/W(1): %s, Success: %s\n",
        s.timeout, s.max_retries, BOOLEAN_TO_STR(s.enable), 
        BOOLEAN_TO_STR(s.write), BOOLEAN_TO_STR(s.success));

    //print_bytes("Watchdog data:", b->resp, PICO_PKT_LEN);     
}

void send_watchdog_read_request(int fd) {
    pico_pkt_watchdog_t s = {0};  
    pkt_buf pkt = {0,0,0};
    memset((void *)&pkt.req, 0, sizeof(pkt.req));
    memset((void *)&pkt.resp, 0, sizeof(pkt.resp));

    s.write = false;
    s.enable = false;
    s.timeout = 0;

    // Pack the watchdog request message
    pico_pkt_watchdog_req_pack((uint8_t *)pkt.req, &s);

    send_serial_port_command(fd, pkt.req, PICO_PKT_LEN);
}

void send_watchdog_write_request(int fd) {
    pico_pkt_watchdog_t s = {0};
    pkt_buf pkt = {0,0,0};
    memset((void *)&pkt.req, 0, sizeof(pkt.req));
    memset((void *)&pkt.resp, 0, sizeof(pkt.resp));

    s.write = true;
    s.enable = appSettings.enable_watchdog_timer;
    s.max_retries = appSettings.pico_watchdog_max_retries;
    s.timeout = appSettings.pico_watchdog_timeout_seconds;
    
    // Pack the watchdog request message
    pico_pkt_watchdog_req_pack((uint8_t *)pkt.req, &s);

    send_serial_port_command(fd, pkt.req, PICO_PKT_LEN);
}

bool send_watchdog_request(int fd, pico_pkt_watchdog_t & s) {
    pkt_buf pkt = {0,0,0};
    memset((void *)&pkt.req, 0, sizeof(pkt.req));
    memset((void *)&pkt.resp, 0, sizeof(pkt.resp));

    // Pack the watchdog request message
    pico_pkt_watchdog_req_pack((uint8_t *)pkt.req, &s);

    send_serial_port_command(fd, pkt.req, PICO_PKT_LEN);
    
    bool success = get_pico_response(fd, pkt, PICO_PKT_WATCHDOG_MAGIC);
    
    if (success) {        
        // Unpack the watchdog response message
        pico_pkt_watchdog_unpack(pkt.resp, &s);
    }
    
    return success;
}

void init_watchdog(int fd) {
    pico_pkt_watchdog_t s = {0};
    pkt_buf pkt = {0,0,0};
    memset((void *)&pkt.req, 0, sizeof(pkt.req));
    memset((void *)&pkt.resp, 0, sizeof(pkt.resp));

    s.write = true;
    s.enable = appSettings.enable_watchdog_timer;
    s.max_retries = appSettings.pico_watchdog_max_retries;
    s.timeout = appSettings.pico_watchdog_timeout_seconds;
    
    send_watchdog_request(fd, s);
}