/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _THERMISTOR_H
#define _THERMISTOR_H

#ifdef __cplusplus
extern "C" {
#endif

// The thermistor with part number ERT-JZEG103FA connected to the 
// ADC has nominal resistance of 10 kΩ at 25°C and 
// B25/50 value of 3380.
#define REFERENCE_TEMPERATURE 25.0f // 25°C
#define CELSIOUS_TO_KELVIN(c) (c + 273.15f) // 0 °C = 273.15 K
#define KELVIN_TO_CELSIOUS(k) (k - 273.15f) // 0 °C = 273.15 K
#define NTC_T0 CELSIOUS_TO_KELVIN(REFERENCE_TEMPERATURE)
#define NTC_R0 10000.0f
#define NTC_B 3380.0f
#define LN_NTC_B 8.125630988f // Natual log of NTC_B

#ifdef __cplusplus
}
#endif

#endif // _THERMISTOR_H