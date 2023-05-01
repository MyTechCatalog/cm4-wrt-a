
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

struct pwm_fan_t pwm_fan[NUM_PWM_FANS] = {
    { .fan_id = SYS_FAN1, .tacho_gpio = FAN1_TACHO_GPIO, .pwm_gpio = FAN1_PWM_GPIO, 
      .prev_tacho_cnt = 0, .tacho_cnt = 0, .rpm = 0, .duty_cycle = 0.5f },

    { .fan_id = CM4_FAN,  .tacho_gpio = CM4_FAN_TACHO_GPIO, .pwm_gpio = CM4_FAN_PWM_GPIO, 
      .prev_tacho_cnt = 0, .tacho_cnt = 0, .rpm = 0, .duty_cycle = 0.5f }    
};

const uint FAN_PWM_COUNT_TOP = 1000;

struct repeating_timer tacho_timer;

bool tachometer_timer_callback(struct repeating_timer *t) {

    for (size_t i = 0; i < NUM_PWM_FANS; i++) {
        uint16_t tacho = (pwm_fan[i].tacho_cnt - pwm_fan[i].prev_tacho_cnt);
        pwm_fan[i].prev_tacho_cnt = pwm_fan[i].tacho_cnt;        
        pwm_fan[i].rpm = tacho * REVS_PER_MSEC_TO_RPM;
    }

    return true;
}

void update_tachometer_counter(uint gpio, uint32_t events){
    for (size_t i = 0; i < NUM_PWM_FANS; i++) {
        if (pwm_fan[i].tacho_gpio == gpio) {
           pwm_fan[i].tacho_cnt++;
           break;
        }        
    }
}

void init_fan_pwm() {
    
    add_repeating_timer_ms(TACHO_TIMER_DELAY_ms, 
    tachometer_timer_callback, NULL, &tacho_timer);

    for (size_t i = 0; i < NUM_PWM_FANS; i++) {
        // Configure PWM slice and set it running    
        pwm_config cfg = pwm_get_default_config();
        pwm_config_set_wrap(&cfg, FAN_PWM_COUNT_TOP);
        pwm_init(pwm_gpio_to_slice_num(pwm_fan[i].pwm_gpio), &cfg, true);
        
        gpio_set_function(pwm_fan[i].pwm_gpio, GPIO_FUNC_PWM);
        pwm_set_gpio_level(pwm_fan[i].pwm_gpio, pwm_fan[i].duty_cycle * (FAN_PWM_COUNT_TOP + 1));
    }    
}

void pkt_fan_pwm(struct pkt_buf *b) {    
    bool write = false;    
    bool success = false;

    struct pico_pkt_fan_pwm_t fans[NUM_PWM_FANS] = {
        { .fan_id = INVALID_FAN_ID, .pwm_pct = 0.0f },
        { .fan_id = INVALID_FAN_ID, .pwm_pct = 0.0f },
    };

    // Unpack the PWM request message
    pico_pkt_fan_pwm_unpack(b->req, fans, &write, &success);
    
    if (write) {        
        for (size_t i = 0; i < NUM_PWM_FANS; i++) {
            if (fans[i].fan_id == pwm_fan[i].fan_id) {
                // Sanitize the input
                float pwm_pct = (fans[i].pwm_pct < 0.0f)? 0.0f : fans[i].pwm_pct;
                pwm_pct = (pwm_pct > 1.0f)? 1.0f : pwm_pct;
                pwm_fan[i].duty_cycle = pwm_pct;
                pwm_set_gpio_level(pwm_fan[i].pwm_gpio, pwm_fan[i].duty_cycle * (FAN_PWM_COUNT_TOP + 1));
                success = true;
            }            
        }
    } else {               
        for (size_t i = 0; i < NUM_PWM_FANS; i++) {
            if (fans[i].fan_id == pwm_fan[i].fan_id) {
                fans[i].pwm_pct = pwm_fan[i].duty_cycle;
                success = true;
            }
        }
    }

    // Pack the PWM response message
    pico_pkt_fan_pwm_resp_pack(b->resp, fans, write, success);

    // Write response to host (blocking)
    uart_write_blocking(UART_ID, b->resp, PICO_PKT_LEN);
}

uint16_t get_fan_rpm(uint8_t fan_id) {
    
    uint16_t rpm = 0;
    if (is_valid_pico_fan_id(fan_id)) {
        for (size_t i = 0; i < NUM_PWM_FANS; i++) {
            if (fan_id == pwm_fan[i].fan_id) {
                rpm = pwm_fan[i].rpm;
                break;
            }
        }
    }

    return rpm;    
}