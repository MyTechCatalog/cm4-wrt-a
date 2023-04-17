/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#include "pkt_handler.h"
#include "pico_pkt_version.h"
#include "cm4-wrt-a.h"

void pkt_version(struct pkt_buf *b) {   
    bool sop = false;   // Start of packet
    bool eop = false;   // End of packet

    pico_pkt_version_resp_pack((uint8_t *)b->resp, &sop, &eop);
    // Write response to host (blocking)
    uart_write_blocking(UART_ID, b->resp, PICO_PKT_LEN);
}

