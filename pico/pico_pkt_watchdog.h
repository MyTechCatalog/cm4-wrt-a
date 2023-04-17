/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PICO_PKT_WATCHDOG_
#define PICO_PKT_WATCHDOG_


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
 * for Watchdog Timer messages. This packet is formatted, as follows: 
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
 * |       3:2      | 16-bit data, little-endian. Timer value in seconds.     |
 * +----------------+---------------------------------------------------------+
 * |       5:4      | 16-bit data, little-endian. Number of restart attempts. |
 * +----------------+---------------------------------------------------------+
 * |      15:6      | Reserved. Set to 0.                                     |
 * +----------------+---------------------------------------------------------+
 *
 *                              Response
 *                      ----------------------
 *
 * The response packet contains the Watchdog timer value as indicated above.
 * A status flag will be set if the operation completed successfully.
 *
 * In the case of a read request, the data field will contain the read data, if
 * the read succeeded.
 *
 * (Note 1)
 *  The flags are defined as follows:
 *
 *    +================+========================+
 *    |      Bit(s)    |         Value          |
 *    +================+========================+
 *    |       7:3      | Reserved. Set to 0.    |
 *    +----------------+------------------------+ 
 *    |        2       |  0 = Disable Watchdog  |
 *    |                |  1 = Enable Watchdog   |
 *    +----------------+------------------------+
 *    |                | Status. Only used in   |
 *    |                | response packet.       |
 *    |                | Ignored in request.    |
 *    |        1       |                        |
 *    |                |   1 = Success          |
 *    |                |   0 = Failure          |
 *    +----------------+------------------------+ 
 *    |        0       |   0 = Read operation   |
 *    |                |   1 = Write operation  |
 *    +----------------+------------------------+
 *
 */

#define PACK_WATCHDOG_U16(timeout,idx)\
    { uint16_t tmpData = timeout;\
    buf[idx]     = tmpData & 0xff;\
    buf[idx + 1] = (tmpData >> 8); }\

#define UNPACK_WATCHDOG_U16(data,idx)\
    data = (buf[idx + 0] << 0) | (buf[idx + 1] << 8);\

/* Request packet indices */
#define PICO_PKT_WATCHDOG_IDX_MAGIC      0
#define PICO_PKT_WATCHDOG_IDX_FLAGS      1
#define PICO_PKT_WATCHDOG_IDX_TIMEOUT    2 /* Watchdog timeout in seconds */
#define PICO_PKT_WATCHDOG_IDX_RETRIES    4 /* Maximum number of restart attempts */
#define PICO_PKT_WATCHDOG_RESV           6

/* Flag bits */
#define PICO_PKT_WATCHDOG_FLAG_WRITE     (1 << 0)
#define PICO_PKT_WATCHDOG_FLAG_SUCCESS   (1 << 1)
#define PICO_PKT_WATCHDOG_ENABLE         (1 << 2)

#define PKT_WATCHDOG { \
    .magic          = PICO_PKT_WATCHDOG_MAGIC, \
    .init           = NULL, \
    .exec           = pkt_watchdog \
}

typedef struct pico_pkt_watchdog_t {
    /// @brief 1 - Write operation, 0 - Read operation
    bool write;

    /// @brief Enable Watchdog (true), Disable Watchdog (false)
    bool enable;

    /// @brief Success (true), Failure (false)
    bool success;

    /// @brief Timeout value in seconds
    uint16_t timeout;

    /// @brief Maximum number of retries
    uint16_t max_retries;    
      
} pico_pkt_watchdog_t;

// Packet handler
void pkt_watchdog(struct pkt_buf *b);
// Called whenever a new message is received form the host (CM4)
// to "reset" count down
void update_watchdog();

/* Pack the request buffer */
/// @brief Packs Watchdog timeout read/write request
/// @param buf Pointer to request buffer
/// @param p request details
static inline void pico_pkt_watchdog_req_pack(uint8_t *buf, 
    pico_pkt_watchdog_t *p) {    
    if (p == NULL) {
        return;
    }

    buf[PICO_PKT_WATCHDOG_IDX_MAGIC] = PICO_PKT_WATCHDOG_MAGIC;
    
    if (p->write) {
        PACK_WATCHDOG_U16(p->timeout, PICO_PKT_WATCHDOG_IDX_TIMEOUT)
        PACK_WATCHDOG_U16(p->max_retries, PICO_PKT_WATCHDOG_IDX_RETRIES)
        buf[PICO_PKT_WATCHDOG_IDX_FLAGS] = PICO_PKT_WATCHDOG_FLAG_WRITE;
    } else {
        buf[PICO_PKT_WATCHDOG_IDX_FLAGS] = 0x00;
    }

    if (p->enable) {
        buf[PICO_PKT_WATCHDOG_IDX_FLAGS] |= PICO_PKT_WATCHDOG_ENABLE;
    } else {
        buf[PICO_PKT_WATCHDOG_IDX_FLAGS] &= ~PICO_PKT_WATCHDOG_ENABLE;
    }

    for (int i = PICO_PKT_WATCHDOG_RESV; i < PICO_PKT_LEN; i++) {
        buf[i] = 0x00;
    }
}

/* Pack the response buffer */
static inline void pico_pkt_watchdog_resp_pack(uint8_t *buf, 
    pico_pkt_watchdog_t *p) {   
    if (p == NULL) {
        return;
    }

    buf[PICO_PKT_WATCHDOG_IDX_MAGIC]     = PICO_PKT_WATCHDOG_MAGIC;
    PACK_WATCHDOG_U16(p->timeout, PICO_PKT_WATCHDOG_IDX_TIMEOUT)
    PACK_WATCHDOG_U16(p->max_retries, PICO_PKT_WATCHDOG_IDX_RETRIES)
    
    if (p->write) {
        buf[PICO_PKT_WATCHDOG_IDX_FLAGS] = PICO_PKT_WATCHDOG_FLAG_WRITE;
    } else {
        buf[PICO_PKT_WATCHDOG_IDX_FLAGS] = 0x00;
    }

    if (p->enable) {
        buf[PICO_PKT_WATCHDOG_IDX_FLAGS] |= PICO_PKT_WATCHDOG_ENABLE;
    } else {
        buf[PICO_PKT_WATCHDOG_IDX_FLAGS] &= ~PICO_PKT_WATCHDOG_ENABLE;
    }

    if (p->success) {
        buf[PICO_PKT_WATCHDOG_IDX_FLAGS] |= PICO_PKT_WATCHDOG_FLAG_SUCCESS;
    } else {
        buf[PICO_PKT_WATCHDOG_IDX_FLAGS] &= ~PICO_PKT_WATCHDOG_FLAG_SUCCESS;
    }

    for (int i = PICO_PKT_WATCHDOG_RESV; i < PICO_PKT_LEN; i++) {
        buf[i] = 0x00;
    }
}

/* Unpack the response buffer */
static inline void pico_pkt_watchdog_unpack(const uint8_t *buf, 
    pico_pkt_watchdog_t *p) { 
    if (p == NULL) {
        return;
    }

    p->write = ((buf[PICO_PKT_WATCHDOG_IDX_FLAGS] & PICO_PKT_WATCHDOG_FLAG_WRITE) != 0);

    UNPACK_WATCHDOG_U16(p->timeout, PICO_PKT_WATCHDOG_IDX_TIMEOUT)
    UNPACK_WATCHDOG_U16(p->max_retries, PICO_PKT_WATCHDOG_IDX_RETRIES)

    p->enable = ((buf[PICO_PKT_WATCHDOG_IDX_FLAGS] & PICO_PKT_WATCHDOG_ENABLE) != 0);    
    
    if ((buf[PICO_PKT_WATCHDOG_IDX_FLAGS] & PICO_PKT_WATCHDOG_FLAG_SUCCESS) != 0) {
        p->success = true;
    } else {
        p->success = false;
    }     
}

#ifndef PICO_BOARD
/// @brief Helper function to send default/startup watchdog settings.
/// @param fd Pico serial file descriptor
void init_watchdog(int fd);

/// @brief Sends watchdog read/write requests to the RPi Pico
/// @param fd Pico serial file descriptor
/// @param s [In/Out] Watchdog request data
/// @return True(1) on success. False(0) on failure.
bool send_watchdog_request(int fd, pico_pkt_watchdog_t & s);
#endif

#ifdef __cplusplus
}
#endif

#endif // PICO_PKT_WATCHDOG_