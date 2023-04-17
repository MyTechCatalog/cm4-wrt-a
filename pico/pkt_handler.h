/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PKT_HANDLER_H_
#define PKT_HANDLER_H_

#ifdef PICO_BOARD
#include "pico/stdlib.h"
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ARRAY_SIZE
#   define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

#define PICO_PKT_LEN 16
#define PKT_MAGIC_IDX 0

typedef struct pkt_buf {
    const uint8_t req[PICO_PKT_LEN];      /* Request */
    uint8_t       resp[PICO_PKT_LEN];     /* Response */
    volatile bool ready;                  /* Ready flag */
} pkt_buf;

struct pkt_handler {
    /**
     * Associates a message from the host with a particular packet handler
     * This may encode information that's meaningful to the packet handler.
     */
    const uint8_t magic;

    /**
     * Perform any required initializations
     */
    void (*init)(void);

    /**
     * Executes packet handler actions. Provided a buffer containing 
     * the request data, the packet handler fills out the response data.
     */
    void (*exec)(struct pkt_buf *b);    
};

#ifdef __cplusplus
}
#endif

#endif
