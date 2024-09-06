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
#include <vector>
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

#define PACK_FAN_PWM(temp_val,offset)\
    buf[PICO_PKT_FAN_PWM_IDX_BASE + offset]     = (uint8_t)(temp_val * FAN_PWM_LSB);\

#define UNPACK_FAN_PWM(data,offset)\
    data = (float)buf[PICO_PKT_FAN_PWM_IDX_BASE + offset];\
    data /= FAN_PWM_LSB;\

/* Request packet indices */
#define PICO_PKT_FAN_PWM_IDX_MAGIC      0
#define PICO_PKT_FAN_PWM_IDX_TARGET_ID  1
#define PICO_PKT_FAN_PWM_IDX_FLAGS      2
#define PICO_PKT_FAN_PWM_IDX_BASE       3 /* Base address of FAN PWM data */
#define PICO_PKT_FAN_PWM_RESV           5

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
#define NUM_PWM_FANS 2

enum pwm_fan_id {
    SYS_FAN1 = 1,
    CM4_FAN  = 2,
    INVALID_FAN_ID
};

#ifdef PICO_BOARD
typedef struct pwm_fan_t {
    /// @brief Fan Identifier
    uint8_t fan_id;

    /// @brief Fan's tachometer GPIO pin
    volatile uint tacho_gpio;

    /// @brief Fan's PWM GPIO pin
    uint pwm_gpio;

    /// @brief Tachometer interrupt Count
    volatile uint64_t tacho_cnt;
    
    /// @brief Prevoius tachometer interrupt Count
    uint64_t prev_tacho_cnt;

    /// @brief Fan Revolutions Per Minute (RPM)
    uint16_t rpm;

    /// @brief FAN1 Pulse Width Modulation (PWM) duty cycle. 
    /// Range: 0.0(0%) to 1.0f(100%)
    float duty_cycle;

} pwm_fan_t;
#endif

typedef struct pico_pkt_fan_pwm_t {
    /// @brief Fan Identifier
    uint8_t fan_id;

    /// @brief Pulse Width Modulation (PWM) setting [0.0 to 1.0]
    float pwm_pct;    
} pico_pkt_fan_pwm_t;

// Packet handler
void pkt_fan_pwm(struct pkt_buf *b);
uint16_t get_fan_rpm(uint8_t fan_id);

void init_fan_pwm();
void turn_off_fan_pwm();

#ifdef PICO_BOARD
// Called by GPIO ISR to update tachometer counter(s)
void update_tachometer_counter(uint gpio, uint32_t events);
#endif

inline bool is_valid_pico_fan_id(uint8_t fan_id) {
    bool ret_val = false;

    switch (fan_id)
    {
    case SYS_FAN1:
    case CM4_FAN:
        ret_val = true;
        break;    
    default:
        break;
    }

    return ret_val;
}

/* Pack the request buffer */
/// @brief Packs FAN PWM read/write request
/// @param buf request buffer
/// @param fan fan struct array
/// @param write True (1) for Write, False (0) to Read operation
static inline void pico_pkt_fan_pwm_req_pack(uint8_t *buf, 
    struct pico_pkt_fan_pwm_t fan[NUM_PWM_FANS], bool write)
{    
    buf[PICO_PKT_FAN_PWM_IDX_MAGIC]     = PICO_PKT_FAN_PWM_MAGIC;
    buf[PICO_PKT_FAN_PWM_IDX_TARGET_ID] = 0x00;
    
    for (size_t i = 0; i < NUM_PWM_FANS; i++) {
        if (!is_valid_pico_fan_id(fan[i].fan_id) || (fan[i].fan_id < 1)) {
            continue;
        }

        buf[PICO_PKT_FAN_PWM_IDX_TARGET_ID] |= (1 << (fan[i].fan_id - 1));
        uint8_t fan_idx = (fan[i].fan_id - 1);        

        PACK_FAN_PWM(fan[i].pwm_pct, fan_idx)
    }
    
    if (write) {        
        buf[PICO_PKT_FAN_PWM_IDX_FLAGS] = PICO_PKT_FAN_PWM_FLAG_WRITE;
    } else {
        buf[PICO_PKT_FAN_PWM_IDX_FLAGS] = 0x00;        
    }

    for (int i = PICO_PKT_FAN_PWM_RESV; i < PICO_PKT_LEN; i++) {
        buf[i] = 0x00;
    }
}

/* Pack the response buffer */
static inline void pico_pkt_fan_pwm_resp_pack(uint8_t *buf, 
    struct pico_pkt_fan_pwm_t fan[NUM_PWM_FANS], bool write, bool success)
{    
    buf[PICO_PKT_FAN_PWM_IDX_MAGIC]     = PICO_PKT_FAN_PWM_MAGIC;
    buf[PICO_PKT_FAN_PWM_IDX_TARGET_ID] = 0x00;

    for (size_t i = 0; i < NUM_PWM_FANS; i++) {
        if (!is_valid_pico_fan_id(fan[i].fan_id) || (fan[i].fan_id < 1)) {
            PACK_FAN_PWM(fan[i].pwm_pct, i)
            continue;
        }
        
        buf[PICO_PKT_FAN_PWM_IDX_TARGET_ID] |= 1 << (fan[i].fan_id - 1);
        uint8_t fan_idx = (fan[i].fan_id - 1);
        PACK_FAN_PWM(fan[i].pwm_pct, fan_idx)
    }
        
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
    struct pico_pkt_fan_pwm_t fan[NUM_PWM_FANS], bool *write, bool *success)
{   
    for (size_t i = 0; i < NUM_PWM_FANS; i++) {
        if( (buf[PICO_PKT_FAN_PWM_IDX_TARGET_ID] & (1 << i)) != 0) {
            fan[i].fan_id = (i+1);           
        }
          
        UNPACK_FAN_PWM(fan[i].pwm_pct, i)
    }
    
    if (write != NULL) {
        *write = ((buf[PICO_PKT_FAN_PWM_IDX_FLAGS] & PICO_PKT_FAN_PWM_FLAG_WRITE) != 0);
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
void init_fan_pwm();

/// @brief Helper function to return RPi Pico PWM FAN ID given its name.
uint8_t get_fan_id_from_name(const char * fan_name);

/// @brief Sends a request to the Pico to read/write FAN PWM setting
/// @param rw_flag Read(0)/Write(1) flag
/// @param fanInfo Fan info.
/// @return True(1) on success. False(0) on failure.
bool send_fan_pwm_request(bool rw_flag, std::vector<struct pico_pkt_fan_pwm_t> &fanInfo);
#endif

#ifdef __cplusplus
}
#endif

#endif //PICO_PKT_FAN_PWM_H_