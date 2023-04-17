/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PICO_PKT_SHUTDOWN_
#define PICO_PKT_SHUTDOWN_


#ifdef PICO_BOARD
#include "pico/stdlib.h"
#else
#include <stdint.h>
#endif

#include "pico_pkt_id.h"
#include "pkt_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

/* This file defines the Host (RPi CM4) <-> Pico (RP2040) packet format
 * for shutdown messages. The Pico sends this packet to the CM4 upon 
 * detecting a button press via one of the RPi Pico's GPIOs. The intent
 * being that the Host (CM4) should initiate a graceful shutdown. 
 * This packet is formatted, as follows: 
 * All values are little-endian.
 *
 *                              Request
 *                      ----------------------
 *
 * +================+=========================================================+
 * |  Byte offset   |                       Description                       |
 * +================+=========================================================+
 * |        0       | Magic Value                                             |
 * +----------------+---------------------------------------------------------+
 * |        1       | Flags (Note 1)                                          |
 * +----------------+---------------------------------------------------------+
 * |      15:2      | Reserved. Set to 0.                                     |
 * +----------------+---------------------------------------------------------+
 *
 *                              Response
 *                      ----------------------
 *
 * No response packet is sent back to the Pico.
 * 
 * (Note 1)
 *  The flags are defined as follows:
 *
 *    +================+========================+
 *    |      Bit(s)    |         Value          |
 *    +================+========================+
 *    |       7:1      | Reserved. Set to 0.    |
 *    +----------------+------------------------+
 *    |                | Status. Only used in   |
 *    |                | response packet.       |
 *    |                | Ignored in request.    |
 *    |        0       |                        |
 *    |                |   1 = Success          |
 *    |                |   0 = Failure          |
 *    +----------------+------------------------+  
 *
 */

/* Request packet indices */
#define PICO_PKT_SHUTDOWN_IDX_MAGIC      0
#define PICO_PKT_SHUTDOWN_IDX_FLAGS      1
#define PICO_PKT_SHUTDOWN_RESV           2

/* Flag bits */
#define PICO_PKT_SHUTDOWN_FLAG_SUCCESS   (1 << 0)

#define PKT_SHUTDOWN { \
    .magic          = PICO_PKT_SHUTDOWN_MAGIC, \
    .init           = NULL, \
    .exec           = pkt_shutdown \
}

// Packet handler
void pkt_shutdown(struct pkt_buf *b);

// Called upon detection of the graceful shutdown button press
void send_shutdown_request();
// Called upon detection of along press of the shutdown button.
// It resets the CM4 by asserting the appropriate pin
void reset_the_cm4();
// Called by GPIO ISR to detect shutdown requests
void detect_shutdown_events(uint32_t events);

/* Pack the request buffer */
/// @brief Packs Watchdog timeout read/write request
/// @param buf Pointer to request buffer
/// @param p request details
static inline void pico_pkt_shutdown_req_pack(uint8_t *buf) {    
    buf[PICO_PKT_SHUTDOWN_IDX_MAGIC] = PICO_PKT_SHUTDOWN_MAGIC;
    buf[PICO_PKT_SHUTDOWN_IDX_FLAGS] = 0x00;

    for (int i = PICO_PKT_SHUTDOWN_RESV; i < PICO_PKT_LEN; i++) {
        buf[i] = 0x00;
    }
}

/* Pack the response buffer */
static inline void pico_pkt_shutdown_resp_pack(uint8_t *buf, bool success) {   
    buf[PICO_PKT_SHUTDOWN_IDX_MAGIC]     = PICO_PKT_SHUTDOWN_MAGIC;
  
    if (success) {
        buf[PICO_PKT_SHUTDOWN_IDX_FLAGS] = PICO_PKT_SHUTDOWN_FLAG_SUCCESS;
    } else {
        buf[PICO_PKT_SHUTDOWN_IDX_FLAGS] = 0x00;
    }

    for (int i = PICO_PKT_SHUTDOWN_RESV; i < PICO_PKT_LEN; i++) {
        buf[i] = 0x00;
    }
}

/* Unpack the message */
static inline void pico_pkt_shutdown_unpack(const uint8_t *buf, bool *success) { 
    if (success != NULL) {
        if ((buf[PICO_PKT_SHUTDOWN_IDX_FLAGS] & PICO_PKT_SHUTDOWN_FLAG_SUCCESS) != 0) {
            *success = true;
        } else {
            *success = false;
        }
    }    
}

#ifdef __cplusplus
}
#endif

#endif // PICO_PKT_SHUTDOWN_