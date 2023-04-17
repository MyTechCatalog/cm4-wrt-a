/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PICO_PKT_TEMPERATURE_H_
#define PICO_PKT_TEMPERATURE_H_

#ifdef PICO_BOARD
#include "pico/stdlib.h"
#else
#include <stdint.h>
#endif

#include "math.h"
#include "pico_pkt_id.h"
#include "pkt_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

/* This file defines the Host (RPi CM4) <-> Pico (RP2040) packet format
 * for board temperature messages. This packet is formatted, as follows: 
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
 * |       3:2      | 16-bit data, little-endian. NTC1 Temperature (°C) x 100 |
 * +----------------+---------------------------------------------------------+
 * |       5:4      | 16-bit data, little-endian. NTC2 Temperature (°C) x 100 |
 * +----------------+---------------------------------------------------------+
 * |       7:6      | 16-bit data, little-endian. NTC3 Temperature (°C) x 100 |
 * +----------------+---------------------------------------------------------+
 * |       9:8      | 16-bit data, little-endian. NTC4 Temperature (°C) x 100 |
 * +----------------+---------------------------------------------------------+
 * |      11:10     | 16-bit data, little-endian. Pico Temperature (°C) x 100 |
 * +----------------+---------------------------------------------------------+
 * |      13:12     | 16-bit data, little-endian. FAN1 Speed (RPM)            |
 * +----------------+---------------------------------------------------------+
 * |      15:14     | Reserved. Set to 0.                                     |
 * +----------------+---------------------------------------------------------+
 *
 *                              Response
 *                      ----------------------
 *
 * The response packet contains the request information.
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

// Number of external temperature sensors
#define NUM_NTC_SENSORS 4

#define TEMPERATURE_LSB 100.0f

#define PACK_TEMPERATURE(temp_val,idx)\
    { uint16_t data;\
    if (temp_val < 0.0f){\
        data = fabs(temp_val) * TEMPERATURE_LSB;\
        data = ~data + 1;\
    } else {\
        data = temp_val * TEMPERATURE_LSB;\
    }\
    buf[idx]     = data & 0xff;\
    buf[idx + 1] = (data >> 8); }\

#define UNPACK_TEMPERATURE(dataOut,idx)\
    { uint16_t data = ((buf[idx + 0] << 0) | (buf[idx + 1] << 8));\
    if (!!(data & 0x8000)){\
        data = ~(data - 1);\
        dataOut = -(float)data;\
    } else {\
        dataOut = (float)((buf[idx + 0] << 0) | (buf[idx + 1] << 8));\
    }\
    dataOut /= TEMPERATURE_LSB; }\

#define PACK_FAN_SPEED(temp_val,idx)\
    { uint16_t data = temp_val;\
    buf[idx]     = data & 0xff;\
    buf[idx + 1] = (data >> 8); }\

#define UNPACK_FAN_SPEED(data,idx)\
    data = ((buf[idx + 0] << 0) | (buf[idx + 1] << 8));

/* Request packet indices */
#define PICO_PKT_TEMPERATURE_IDX_MAGIC      0
#define PICO_PKT_TEMPERATURE_IDX_FLAGS      1
#define PICO_PKT_TEMPERATURE_IDX_NTC1       2 /* NTC thermistor 1 temperature in °C x 100 */
#define PICO_PKT_TEMPERATURE_IDX_NTC2       4 /* NTC thermistor 2 temperature in °C x 100 */
#define PICO_PKT_TEMPERATURE_IDX_NTC3       6 /* NTC thermistor 3 temperature in °C x 100 */
#define PICO_PKT_TEMPERATURE_IDX_NTC4       8 /* NTC thermistor 4 temperature in °C x 100 */
#define PICO_PKT_TEMPERATURE_IDX_SELF       10 /* Pico onboard temperature sensor in °C x 100 */
#define PICO_PKT_IDX_FAN1_SPEED             12 /* System FAN 1 speed in Revolutions Per Minute (RPM) */
#define PICO_PKT_TEMPERATURE_RESV           14

/* Flag bits */
#define PICO_PKT_TEMPERATURE_FLAG_SUCCESS   (1 << 0)

#define PKT_TEMPERATURE { \
    .magic          = PICO_PKT_TEMPERATURE_MAGIC, \
    .init           = NULL, \
    .exec           = pkt_temperature \
}

typedef union pico_pkt_temperature_u
{
    struct pico_pkt_temperature_t {
        float ntc1; // NTC thermistor 1 temperature in °C x 100
        float ntc2; // NTC thermistor 1 temperature in °C x 100
        float ntc3; // NTC thermistor 1 temperature in °C x 100
        float ntc4; // NTC thermistor 1 temperature in °C x 100
        float pico; // Pico onboard temperature sensor in °C x 100
        uint32_t fan1rpm; // System FAN 1 speed in Revolutions Per Minute (RPM)
    } s;
    // Add two (2) for the Pico's onboard temperature sensor, and
    // FAN 1 speed.
    float data[NUM_NTC_SENSORS + 2];
} pico_pkt_temperature_u;



// FAN1 revolutions per minute
extern uint16_t fan1Tacho;

// Packet handler
void pkt_temperature(struct pkt_buf *b);

/* Pack the request buffer */
static inline void pico_pkt_temperature_req_pack(uint8_t *buf)
{    
    buf[PICO_PKT_TEMPERATURE_IDX_MAGIC] = PICO_PKT_TEMPERATURE_MAGIC;
    buf[PICO_PKT_TEMPERATURE_IDX_FLAGS] = 0x00;

    buf[PICO_PKT_TEMPERATURE_IDX_NTC1]     = 0x00;
    buf[PICO_PKT_TEMPERATURE_IDX_NTC1 + 1] = 0x00;
    buf[PICO_PKT_TEMPERATURE_IDX_NTC2]     = 0x00;
    buf[PICO_PKT_TEMPERATURE_IDX_NTC2 + 1] = 0x00;
    buf[PICO_PKT_TEMPERATURE_IDX_NTC3]     = 0x00;
    buf[PICO_PKT_TEMPERATURE_IDX_NTC3 + 1] = 0x00;
    buf[PICO_PKT_TEMPERATURE_IDX_NTC4]     = 0x00;
    buf[PICO_PKT_TEMPERATURE_IDX_NTC4 + 1] = 0x00;
    buf[PICO_PKT_TEMPERATURE_IDX_SELF]     = 0x00;
    buf[PICO_PKT_TEMPERATURE_IDX_SELF + 1] = 0x00;
    buf[PICO_PKT_IDX_FAN1_SPEED + 0]       = 0x00;
    buf[PICO_PKT_IDX_FAN1_SPEED + 1]       = 0x00;
    buf[PICO_PKT_TEMPERATURE_RESV + 0]     = 0x00;
    buf[PICO_PKT_TEMPERATURE_RESV + 1]     = 0x00;
}

/* Pack the response buffer */
static inline void pico_pkt_temperature_resp_pack(uint8_t *buf, 
    pico_pkt_temperature_u* p, bool success)
{    
    buf[PICO_PKT_TEMPERATURE_IDX_MAGIC] = PICO_PKT_TEMPERATURE_MAGIC;
    buf[PICO_PKT_TEMPERATURE_IDX_FLAGS] = 0x00;

    PACK_TEMPERATURE(p->s.ntc1, PICO_PKT_TEMPERATURE_IDX_NTC1)
    PACK_TEMPERATURE(p->s.ntc2, PICO_PKT_TEMPERATURE_IDX_NTC2)
    PACK_TEMPERATURE(p->s.ntc3, PICO_PKT_TEMPERATURE_IDX_NTC3)
    PACK_TEMPERATURE(p->s.ntc4, PICO_PKT_TEMPERATURE_IDX_NTC4)
    PACK_TEMPERATURE(p->s.pico, PICO_PKT_TEMPERATURE_IDX_SELF)
    PACK_FAN_SPEED(p->s.fan1rpm, PICO_PKT_IDX_FAN1_SPEED)
    
    buf[PICO_PKT_TEMPERATURE_RESV + 0] = 0x00;
    buf[PICO_PKT_TEMPERATURE_RESV + 1] = 0x00;

    if (success) {
        buf[PICO_PKT_TEMPERATURE_IDX_FLAGS] |= PICO_PKT_TEMPERATURE_FLAG_SUCCESS;
    }
}

/* Unpack the response buffer */
static inline void pico_pkt_temperature_resp_unpack(const uint8_t *buf, 
    pico_pkt_temperature_u* p, bool *success)
{   
    UNPACK_TEMPERATURE(p->s.ntc1, PICO_PKT_TEMPERATURE_IDX_NTC1)
    UNPACK_TEMPERATURE(p->s.ntc2, PICO_PKT_TEMPERATURE_IDX_NTC2)
    UNPACK_TEMPERATURE(p->s.ntc3, PICO_PKT_TEMPERATURE_IDX_NTC3)
    UNPACK_TEMPERATURE(p->s.ntc4, PICO_PKT_TEMPERATURE_IDX_NTC4)
    UNPACK_TEMPERATURE(p->s.pico, PICO_PKT_TEMPERATURE_IDX_SELF)
    UNPACK_FAN_SPEED(p->s.fan1rpm, PICO_PKT_IDX_FAN1_SPEED)

    if ((buf[PICO_PKT_TEMPERATURE_IDX_FLAGS] & PICO_PKT_TEMPERATURE_FLAG_SUCCESS) != 0) {
        *success = true;
    } else {
        *success = false;
    } 
}

// Host (CM4) function definitions
#ifndef PICO_BOARD
/// @brief Sends a board temperature request to the RPi Pico
/// @param fd Pico serial file descriptor
/// @param tmp [out] board temperatures
/// @return True(1) on success. False(0) on failure
bool send_temperature_request(int fd, pico_pkt_temperature_u & tmp);
#endif

#ifdef __cplusplus
}
#endif

#endif //PICO_PKT_TEMPERATURE_H_