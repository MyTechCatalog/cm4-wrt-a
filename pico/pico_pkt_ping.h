/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PICO_PKT_PING_H_
#define PICO_PKT_PING_H_

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
 * for board echo ping messages. This packet is formatted, as follows: 
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
 * |      15:1      | Data.                                                   |
 * +----------------+---------------------------------------------------------+
 *
 *                              Response
 *                      ----------------------
 *
 * The response packet contains the same information as the request.
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

#define PACK_16(data,idx)\
    buf[idx]     = data & 0xff;\
    buf[idx + 1] = (data >> 8);

#define UNPACK_16(data0,data1) ((data0 << 0) | (data1 << 8))

/* Request packet indices */
#define PICO_PKT_PING_IDX_MAGIC      0
#define PICO_PKT_PING_IDX_FLAGS      1
#define PICO_PKT_PING_RESV           2

/* Flag bits */
#define PICO_PKT_PING_FLAG_SUCCESS   (1 << 0)

#define PKT_PING { \
    .magic          = PICO_PKT_PING_MAGIC, \
    .init           = NULL, \
    .exec           = pkt_ping \
}

void pkt_ping(struct pkt_buf *b);

/* Pack the request buffer */
static inline void pico_pkt_ping_req_pack(uint8_t *buf)
{
    static uint8_t cnt = 0;

    buf[PICO_PKT_PING_IDX_MAGIC] = PICO_PKT_PING_MAGIC;
    buf[PICO_PKT_PING_IDX_FLAGS] = 0x00;

    for (size_t i = PICO_PKT_PING_RESV; i < PICO_PKT_LEN; i++) {
        buf[i] = (cnt+1);
    }

    cnt++;
}

/* Pack the response buffer */
static inline void pico_pkt_ping_resp_pack(uint8_t *buf, bool success)
{   
    buf[PICO_PKT_PING_IDX_MAGIC] = PICO_PKT_PING_MAGIC;
   
    if (success) {
        buf[PICO_PKT_PING_IDX_FLAGS] = PICO_PKT_PING_FLAG_SUCCESS;
    }
}

/* Unpack the response buffer */
static inline void pico_pkt_ping_resp_unpack(const uint8_t *buf,
    bool *success)
{
    if ((buf[PICO_PKT_PING_IDX_FLAGS] & PICO_PKT_PING_FLAG_SUCCESS) != 0) {
        *success = true;
    } else {
        *success = false;
    } 
}

// Host (CM4) function definitions
#ifndef PICO_BOARD
void ping_the_pico(int fd);
int time_the_pico_ping();
void pkt_simulate_pico_ping_reponse(struct pkt_buf *b);
#endif

#ifdef __cplusplus
}
#endif

#endif //PICO_PKT_PING_H_