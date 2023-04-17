/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PICO_PKT_
#define PICO_PKT_

/* This file defines the Host (RPi CM4) <-> Pico (RP2040) packet IDs
 * (magic numbers) for all supported messages. *
 */

#define PICO_PKT_TEMPERATURE_MAGIC      ((uint8_t) 'A')
#define PICO_PKT_FAN_PWM_MAGIC          ((uint8_t) 'B')
#define PICO_PKT_WATCHDOG_MAGIC         ((uint8_t) 'C')
#define PICO_PKT_SHUTDOWN_MAGIC         ((uint8_t) 'D')
#define PICO_PKT_PING_MAGIC             ((uint8_t) 'E')
#define PICO_PKT_VERSION_MAGIC          ((uint8_t) 'F')

#endif // PICO_PKT_