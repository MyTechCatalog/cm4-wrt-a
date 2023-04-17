
/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "cm4-wrt-a.h"
#include "pkt_handler.h"
#include "pico_pkt_fan_pwm.h"

// FAN1 Tachometer interrupt Counter
volatile static uint64_t fan1Count = 0;

// FAN1 revolutions per minute
uint16_t fan1Tacho = 0;

const uint FAN_PWM_COUNT_TOP = 1000;

/// @brief FAN1 Pulse Width Modulation (PWM) duty cycle. 
/// Range: 0.0(0%) to 1.0f(100%)
float fan1_pwm_duty_cycle = 0.5f;

struct repeating_timer tacho_timer;

bool tachometer_timer_callback(struct repeating_timer *t) {
    static uint64_t prevFan1Tacho = 0;
    fan1Tacho = (fan1Count - prevFan1Tacho);
    prevFan1Tacho = fan1Count;
    // FAN1 revolutions per minute
    fan1Tacho = fan1Tacho * REVS_PER_MSEC_TO_RPM;
    return true;
}

void update_tachometer_counter(uint32_t events){
    fan1Count++;
}

void init_fan_pwm(uint gpio) {
    add_repeating_timer_ms(TACHO_TIMER_DELAY_ms, 
        tachometer_timer_callback, NULL, &tacho_timer);

    // Configure PWM slice and set it running    
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_wrap(&cfg, FAN_PWM_COUNT_TOP);
    pwm_init(pwm_gpio_to_slice_num(gpio), &cfg, true);
    
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    pwm_set_gpio_level(gpio, fan1_pwm_duty_cycle * (FAN_PWM_COUNT_TOP + 1));
}

void pkt_fan_pwm(struct pkt_buf *b){
    uint8_t fan_id = 0;
    bool write = false;
    float pwm_pct = 0.0f;
    bool success = false;
    
    // Unpack the PWM request message
    pico_pkt_fan_pwm_unpack(b->req, &fan_id, &write, &pwm_pct, &success);
    
    if((fan_id == 1) && write) {
        success = true;
        // Sanitize the input
        pwm_pct = (pwm_pct < 0.0f)? 0.0f : pwm_pct;
        pwm_pct = (pwm_pct > 1.0f)? 1.0f : pwm_pct;
        fan1_pwm_duty_cycle = pwm_pct;
        pwm_set_gpio_level(FAN1_PWM_GPIO, fan1_pwm_duty_cycle * (FAN_PWM_COUNT_TOP + 1));
    } else if((fan_id == 1) && !write) {
        success = true;       
        pwm_pct = fan1_pwm_duty_cycle;
    } else {
        success = false;
    }

    // Pack the PWM response message
    pico_pkt_fan_pwm_resp_pack(b->resp, fan_id, write, pwm_pct, success);

    // Write response to host (blocking)
    uart_write_blocking(UART_ID, b->resp, PICO_PKT_LEN);
}