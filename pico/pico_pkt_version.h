/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PICO_PKT_VERSION_H_
#define PICO_PKT_VERSION_H_

#ifdef PICO_BOARD
#include "pico/stdlib.h"
#else
#include <stdint.h>
#include <stdlib.h>
#endif

#include "pico_pkt_id.h"
#include "pkt_handler.h"
#include <string.h>
#include "version.h"

#ifdef __cplusplus
extern "C" {
#endif

/* This file defines the Host (RPi CM4) <-> Pico (RP2040) packet format
 * for Pico build version messages. This packet is formatted, as follows: 
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
 * |      15:1      | Version string.                                         |
 * +----------------+---------------------------------------------------------+
 *
 *                              Response
 *                      ----------------------
 *
 * The response packet contains the Pico's (Git) build version string.
 * This may require multiple packets to be sent. In which case, the
 * first packet will have the Start of Packet (SOP) status flag set 
 * while the last packet will have the End of Packet (EOP) status flag 
 * set.
 *
 * (Note 1)
 *  The flags are defined as follows:
 *
 *    +================+========================+
 *    |      Bit(s)    |         Value          |
 *    +================+========================+
 *    |       7:2      | Reserved. Set to 0.    |
 *    +----------------+------------------------+
 *    |                | Status. Only used in   |
 *    |                | response packet.       |
 *    |                | Ignored in request.    |
 *    |        1       |                        |
 *    |                | 1 = End of Packet      |
 *    |                | 0 = Not EOP            |
 *    +----------------+------------------------+
 *    |                | Status. Only used in   |
 *    |                | response packet.       |
 *    |                | Ignored in request.    |
 *    |        0       |                        |
 *    |                | 1 = Start of Packet    |
 *    |                | 0 = Not SOP            |
 *    +----------------+------------------------+ 
 *
 */


/* Request packet indices */
#define PICO_PKT_VERSION_IDX_MAGIC      0
#define PICO_PKT_VERSION_IDX_FLAGS      1
#define PICO_PKT_VERSION_DATA           2

/* Flag bits */
#define PICO_PKT_VERSION_FLAG_SOP   (1 << 0)
#define PICO_PKT_VERSION_FLAG_EOP   (1 << 1)

#define PKT_VERSION { \
    .magic          = PICO_PKT_VERSION_MAGIC, \
    .init           = NULL, \
    .exec           = pkt_version \
}

void pkt_version(struct pkt_buf *b);

/* Pack the request buffer */
static inline void pico_pkt_version_req_pack(uint8_t *buf) {
    buf[PICO_PKT_VERSION_IDX_MAGIC] = PICO_PKT_VERSION_MAGIC;
    buf[PICO_PKT_VERSION_IDX_FLAGS] = 0x00;

    for (size_t i = PICO_PKT_VERSION_DATA; i < PICO_PKT_LEN; i++) {
        buf[i] = 0;
    }
}

/// @brief Packs the Pico version string into the response buffer
/// This function gets called repeatedly to send the version
/// string to the host in two or more packets
/// @param buf Response buffer
/// @param sop Flag set to true at start of packet.
/// @param eop Flag set to true at end of packet.
static inline void pico_pkt_version_resp_pack(uint8_t *buf, 
    bool *sop, bool *eop) {
    const char ver[] = VERSION_STR;
    static size_t dataIndex = 0;

    *sop = (dataIndex==0) ? true : false;

    buf[PICO_PKT_VERSION_IDX_MAGIC] = PICO_PKT_VERSION_MAGIC;
    buf[PICO_PKT_VERSION_IDX_FLAGS] = 0x00;
       
    for (size_t i = PICO_PKT_VERSION_DATA; i < PICO_PKT_LEN; i++) {
        buf[i] = ver[dataIndex];
        if (ver[dataIndex]==0){
            break;
        }        
        dataIndex++;
    }

    *eop = (ver[dataIndex]==0) ? true : false;
    
    if (*eop) {
        buf[PICO_PKT_VERSION_IDX_FLAGS] = PICO_PKT_VERSION_FLAG_EOP;
        dataIndex = 0;
    }

    if (*sop) {
        buf[PICO_PKT_VERSION_IDX_FLAGS] |= PICO_PKT_VERSION_FLAG_SOP;
    }
}

// Host (CM4) function definitions
#ifndef PICO_BOARD
/// @brief Unpacks the Pico version string into the response buffer
/// This function gets called repeatedly to concatenate the version
/// string on the CM4 host
/// @param buf Response buffer
/// @param sop Flag set to true at start of packet.
/// @param eop Flag set to true at end of packet.
/// @returns Pointer to version string chunk. Caller is responsible 
/// for calling free() on the returned string
static inline char * pico_pkt_version_resp_unpack(const uint8_t *buf,
    bool *sop, bool *eop) {
    
    if (eop) {
        if ((buf[PICO_PKT_VERSION_IDX_FLAGS] & PICO_PKT_VERSION_FLAG_EOP) != 0) {
            *eop = true;
        } else {
            *eop = false;
        }
    }

    if (sop) {
        if ((buf[PICO_PKT_VERSION_IDX_FLAGS] & PICO_PKT_VERSION_FLAG_SOP) != 0) {
            *sop = true;
        } else {
            *sop = false;
        }
    }

    char * dataOut = (char*)calloc(PICO_PKT_LEN + 1, sizeof(char));
    if (!dataOut) {
        return dataOut;
    }
        
    for (size_t j = 0, i = PICO_PKT_VERSION_DATA; i < PICO_PKT_LEN; i++, j++) {
        
        dataOut[j] = buf[i];
        
        if (buf[i] == 0) {
            break;
        }
    }

    return dataOut;
}

#endif //@END #ifndef PICO_BOARD

#ifdef __cplusplus
}
#endif

#endif //PICO_PKT_VERSION_H_