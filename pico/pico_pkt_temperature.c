/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#include "thermistor.h"
#include "pkt_handler.h"
#include "pico_pkt_temperature.h"
#include "pico_pkt_fan_pwm.h"
#include "hardware/adc.h"
#include "cm4-wrt-a.h"

static pico_pkt_temperature_u temp_data;

/*! @brief Calculates the temperature from ADC value.
* Reads the ADC value associated with the specified 
* Negative Temperature Coefficient (NTC) thermistor
* sensor, and calculates the temperature in degrees Celsius
* \return Temperature in degrees Celsius
*/
static inline float get_ntc_temperature(uint input)
{
    float temp_out = 0.0f;
    // Select ADC input
    adc_select_input(input);
    
    // 12-bit conversion, assume max value == ADC_VREF == 3.3 V
    const float conversion_factor = 3.3f / (1 << 12);
    uint16_t result = adc_read();
    float voltage = result * conversion_factor;
    // The analog-to-digital converter measures the voltage at 
    // the midpoint of a divider consisting of a known resistance of 
    // 10kohm between Vcc(3.3V) and a thermistor of unknown resistance. 
    // We can thus calculate the thermistor resistance from 
    // the measured voltage as:

    const float known_res = 10e3f;
    float thermistor_res = known_res * (1.0f/((3.3f/voltage) - 1.0f));
    
    // Ref: https://en.wikipedia.org/wiki/Thermistor
    float temp_kelvin = NTC_B / (log(thermistor_res) - LN_NTC_B + (NTC_B / NTC_T0));
    temp_out = KELVIN_TO_CELSIOUS(temp_kelvin) + REFERENCE_TEMPERATURE;

    return temp_out;    
}

/* References for this implementation:
 * raspberry-pi-pico-c-sdk.pdf, Section '4.1.1. hardware_adc'
 * pico-examples/adc/adc_console/adc_console.c */
static inline float read_onboard_temperature() {
    
    /* 12-bit conversion, assume max value == ADC_VREF == 3.3 V */
    const float conversionFactor = 3.3f / (1 << 12);
    
    // Select ADC input
    adc_select_input(4);
    
    float adc = (float)adc_read() * conversionFactor;
    float tempC = 27.0f - (adc - 0.706f) / 0.001721f;
    return tempC;
}

void pkt_temperature(struct pkt_buf *b)
{    
    bool success = true;

    for (int ch=0; ch < NUM_NTC_SENSORS; ch++) {
        temp_data.data[ch] = get_ntc_temperature(ch);
    }

    temp_data.s.pico = read_onboard_temperature();
    temp_data.s.fan1rpm = get_fan_rpm(SYS_FAN1);
    temp_data.s.cm4_fan_rpm = get_fan_rpm(CM4_FAN);
   
    pico_pkt_temperature_resp_pack((uint8_t *)b->resp, &temp_data, success);
    // Write response to host (blocking)
    uart_write_blocking(UART_ID, b->resp, PICO_PKT_LEN);
}