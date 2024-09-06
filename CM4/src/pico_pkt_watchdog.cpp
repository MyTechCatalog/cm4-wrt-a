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
#include "pico_pkt_watchdog.h"
#include "PacketHandler.hpp"

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

bool send_watchdog_request(pico_pkt_watchdog_t & s) {
    pkt_buf pkt = {0,0,0};
    memset((void *)&pkt.req, 0, sizeof(pkt.req));
    memset((void *)&pkt.resp, 0, sizeof(pkt.resp));

    // Pack the watchdog request message
    pico_pkt_watchdog_req_pack((uint8_t *)pkt.req, &s);

    PacketHandler::instance().send_pico_request(pkt.req, PICO_PKT_LEN);
    
    bool success = PacketHandler::instance().get_pico_response(pkt, PICO_PKT_WATCHDOG_MAGIC);
    
    memset((void *)&s, 0, sizeof(s));
    
    if (success) {
        // Unpack the watchdog response message
        pico_pkt_watchdog_unpack(pkt.resp, &s);
        success = s.success;
    }
    
    return success;
}

void init_watchdog() {
    pico_pkt_watchdog_t s = {0};
    pkt_buf pkt = {0,0,0};
    memset((void *)&pkt.req, 0, sizeof(pkt.req));
    memset((void *)&pkt.resp, 0, sizeof(pkt.resp));

    s.write = true;
    s.enable = appSettings.enable_watchdog_timer;
    s.max_retries = appSettings.pico_watchdog_max_retries;
    s.timeout = appSettings.pico_watchdog_timeout_seconds;
    
    send_watchdog_request(s);
}