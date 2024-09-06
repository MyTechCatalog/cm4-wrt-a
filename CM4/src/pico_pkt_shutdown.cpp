/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#include "Utils.hpp"
#include "pico_pkt_shutdown.h"
#include "pico_pkt_fan_pwm.h"
#include "PacketHandler.hpp"

void pkt_shutdown(struct pkt_buf *b){
    //print_bytes("Shutdown Request:", b->resp, PICO_PKT_LEN);
    print_err("Shutdown Request\n");

    // Turn off the PWM fans.
    turn_off_fan_pwm();

    // Send back a confirmation of graceful shutdown
    pkt_buf pkt = {0,0,0};
    memset((void *)&pkt.req, 0, sizeof(pkt.req));
    memset((void *)&pkt.resp, 0, sizeof(pkt.resp));

    pico_pkt_shutdown_req_pack((uint8_t *)pkt.req);

    PacketHandler::instance().send_pico_request(pkt.req, PICO_PKT_LEN);
    
    if (isOpenWrt()) {
        run_cmd((char *)"poweroff");
    } else {
        run_cmd((char *)"sudo poweroff");
    }    
}
