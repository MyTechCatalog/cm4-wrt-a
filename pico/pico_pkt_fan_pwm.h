/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PICO_PKT_FAN_PWM_H_
#define PICO_PKT_FAN_PWM_H_

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
 * for fan Pulse Width Modulation (PWM) messages. This packet is formatted, 
 * as follows: 
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
 * |        1       | FAN ID Flags: Set bit N-1 to target FAN N               |
 * +----------------+---------------------------------------------------------+
 * |        2       | Flags (Note 1)                                          |
 * +----------------+---------------------------------------------------------+
 * |       N:3      | 8-bit data x N, for FANs 1 to N PWM [0 to 100]%         |
 * +----------------+---------------------------------------------------------+
 * |      15:N      | Reserved. Set to 0.                                     |
 * +----------------+---------------------------------------------------------+
 *
 *                              Response
 *                      ----------------------
 *
 * The response packet contains the PWM value(s) as indicated above.
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
 *    |       7:2      | Reserved. Set to 0.    |
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

#define FAN_PWM_LSB 100.0f

#define PACK_FAN_PWM(temp_val,idx)\
    buf[idx]     = (uint8_t)(temp_val * FAN_PWM_LSB);\

#define UNPACK_FAN_PWM(data,idx)\
    data = (float)buf[idx];\
    data /= FAN_PWM_LSB;\

/* Request packet indices */
#define PICO_PKT_FAN_PWM_IDX_MAGIC      0
#define PICO_PKT_FAN_PWM_IDX_TARGET_ID  1
#define PICO_PKT_FAN_PWM_IDX_FLAGS      2
#define PICO_PKT_FAN_PWM_IDX_FAN1       3 /* FAN1 PWM value 0 to 100 (%) */
#define PICO_PKT_FAN_PWM_RESV           4

/* Flag bits */
#define PICO_PKT_FAN_PWM_FLAG_WRITE     (1 << 0)
#define PICO_PKT_FAN_PWM_FLAG_SUCCESS   (1 << 1)

#define PKT_FAN_PWM { \
    .magic          = PICO_PKT_FAN_PWM_MAGIC, \
    .init           = NULL, \
    .exec           = pkt_fan_pwm \
}

// Period of timer used to calculate FAN speed
#define TACHO_TIMER_DELAY_ms 1000
#define REVS_PER_MSEC_TO_RPM 60

// Packet handler
void pkt_fan_pwm(struct pkt_buf *b);

#ifdef PICO_BOARD
void init_fan_pwm(uint gpio);
// Called by GPIO ISR to update tachometer counter(s)
void update_tachometer_counter(uint32_t events);
#endif

/* Pack the request buffer */
/// @brief Packs FAN PWM read/write request
/// @param fan_id Number 1 to N to indicating target fan
/// @param write True for Write, False to Read operation
/// @param pwm_pct PWM value (0 to 100)
static inline void pico_pkt_fan_pwm_req_pack(uint8_t *buf, 
    uint8_t fan_id, bool write, float pwm_pct)
{    
    buf[PICO_PKT_FAN_PWM_IDX_MAGIC] = PICO_PKT_FAN_PWM_MAGIC;
    buf[PICO_PKT_FAN_PWM_IDX_TARGET_ID] = 1 << (fan_id-1);
    
    if (write) {
        PACK_FAN_PWM(pwm_pct, PICO_PKT_FAN_PWM_IDX_FAN1)
        buf[PICO_PKT_FAN_PWM_IDX_FLAGS] = PICO_PKT_FAN_PWM_FLAG_WRITE;
    } else {
        buf[PICO_PKT_FAN_PWM_IDX_FLAGS] = 0x00;
        buf[PICO_PKT_FAN_PWM_IDX_FAN1]      = 0x00;
    }

    for (int i = PICO_PKT_FAN_PWM_RESV; i < PICO_PKT_LEN; i++) {
        buf[i] = 0x00;
    }
}

/* Pack the response buffer */
static inline void pico_pkt_fan_pwm_resp_pack(uint8_t *buf, 
    uint8_t fan_id, bool write, float pwm_pct, bool success)
{    
    buf[PICO_PKT_FAN_PWM_IDX_MAGIC]     = PICO_PKT_FAN_PWM_MAGIC;
    buf[PICO_PKT_FAN_PWM_IDX_TARGET_ID] = 1 << (fan_id-1);    
    PACK_FAN_PWM(pwm_pct, PICO_PKT_FAN_PWM_IDX_FAN1)
    
    if (write) {
        buf[PICO_PKT_FAN_PWM_IDX_FLAGS] = PICO_PKT_FAN_PWM_FLAG_WRITE;
    } else {
        buf[PICO_PKT_FAN_PWM_IDX_FLAGS] = 0x00;
    }

    if (success) {
        buf[PICO_PKT_FAN_PWM_IDX_FLAGS] |= PICO_PKT_FAN_PWM_FLAG_SUCCESS;
    } else {
        buf[PICO_PKT_FAN_PWM_IDX_FLAGS] &= ~PICO_PKT_FAN_PWM_FLAG_SUCCESS;
    }

    for (int i = PICO_PKT_FAN_PWM_RESV; i < PICO_PKT_LEN; i++) {
        buf[i] = 0x00;
    }
}

/* Unpack the response buffer */
static inline void pico_pkt_fan_pwm_unpack(const uint8_t *buf, 
    uint8_t *fan_id, bool *write, float *pwm_pct, bool *success)
{   
    if (fan_id != NULL) {
        for (uint8_t i = 0; i < 8; i++) {
            if( (buf[PICO_PKT_FAN_PWM_IDX_TARGET_ID] & (1 << i)) != 0) {
                *fan_id = (i+1);
                break; // Only one FAN is present for now
            }
        }
    }

    if (write != NULL) {
        *write = ((buf[PICO_PKT_FAN_PWM_IDX_FLAGS] & PICO_PKT_FAN_PWM_FLAG_WRITE) != 0);
    }

    if (pwm_pct != NULL) {
        UNPACK_FAN_PWM(*pwm_pct, PICO_PKT_FAN_PWM_IDX_FAN1)
    }
    
    if (success != NULL) {
        if ((buf[PICO_PKT_FAN_PWM_IDX_FLAGS] & PICO_PKT_FAN_PWM_FLAG_SUCCESS) != 0) {
            *success = true;
        } else {
            *success = false;
        }
    }     
}

// Host (CM4) function definitions
#ifndef PICO_BOARD
/// @brief Helper function to send default/startup PWM FAN settings.
/// @param fd Pico serial file descriptor
void init_fan_pwm(int fd);

/// @brief Sends a request to the Pico to read/write FAN PWM setting
/// @param fd Pico serial file descriptor
/// @param rw_flag Read(0)/Write(1) flag
/// @param fan_id Identifier of FAN.
/// @param pwm_pct Pulse Width Modulation (PWM) setting [0.0 to 1.0]
/// @return True(1) on success. False(0) on failure.
bool send_fan_pwm_request(int fd, bool rw_flag, uint8_t fan_id, float & pwm_pct);
#endif

#ifdef __cplusplus
}
#endif

#endif //PICO_PKT_FAN_PWM_H_