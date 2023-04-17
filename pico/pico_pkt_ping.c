/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#include "pkt_handler.h"
#include "pico_pkt_ping.h"
#include "cm4-wrt-a.h"

void pkt_ping(struct pkt_buf *b) {   
    bool success = true;

    for (int i = PICO_PKT_PING_RESV; i < PICO_PKT_LEN; i++) {
        b->resp[i] = b->req[i];
    }
    
    pico_pkt_ping_resp_pack((uint8_t *)b->resp, success);
    // Write response to host (blocking)
    uart_write_blocking(UART_ID, b->resp, PICO_PKT_LEN);
}

