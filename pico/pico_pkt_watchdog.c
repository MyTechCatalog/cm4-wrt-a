/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#include "pico/stdlib.h"
#include "pkt_handler.h"
#include "pico_pkt_watchdog.h"
#include "pico_pkt_shutdown.h"
#include "cm4-wrt-a.h"

// Watchdog timeout for CM4. 
static uint16_t cm4_watchdog_timeout_sec = 30;
// Flag indicating whether the CM4 watchdog is enabled (true) 
static volatile bool is_cm4_watchdog_enabled = false;
extern bool is_host_shutdown_request_pending;
static uint16_t max_retries = 0;
static struct repeating_timer cm4_watchdog_timer;
// System time when the last message from the host was received
uint64_t lastHostMessageTime = 0;
static pico_pkt_watchdog_t s = {0};

void update_watchdog() {
    lastHostMessageTime = get_time();
}

bool cm4_watchdog_timer_callback(struct repeating_timer *t) {
    if (!is_cm4_watchdog_enabled){
        return false;
    }

    uint64_t currentTime = get_time();
    uint64_t diff = currentTime - lastHostMessageTime;
    uint64_t timeout_us = cm4_watchdog_timeout_sec * 1000000LL;
    // Assume CM4 (picod) is alive   
    bool is_CM4_alive = true;

    if (diff >= timeout_us) {
        is_CM4_alive = false;
    }

    if (!is_CM4_alive && !is_host_shutdown_request_pending) {
        // Reset the CM4 host
        gpio_put(LED_PIN3_GPIO, 0);
        reset_the_cm4();
        gpio_put(LED_PIN3_GPIO, 1);
        is_host_shutdown_request_pending = true;
        if (max_retries > 0) {
            max_retries--;
        } else {
            is_cm4_watchdog_enabled = false;
        }
    }

    return true;
}

void pkt_watchdog(struct pkt_buf *b){
        
    // Unpack the Watchdog request message
    pico_pkt_watchdog_unpack(b->req, &s);
    
    s.success = true;

    if (s.write) {        
        max_retries = s.max_retries;
        cm4_watchdog_timeout_sec = s.timeout;
    } else {       
        s.max_retries = max_retries;
        s.timeout = cm4_watchdog_timeout_sec;
    }

    if (s.enable) {
        if (s.write && !is_cm4_watchdog_enabled) {
            uint32_t timout_ms = 100;            
            // Start the timer          
            is_cm4_watchdog_enabled = add_repeating_timer_ms(timout_ms, 
                cm4_watchdog_timer_callback, NULL, &cm4_watchdog_timer);
            s.success = is_cm4_watchdog_enabled;
            /*if (is_cm4_watchdog_enabled) {
                // Turn ON(0) Pico LED1
                gpio_put(LED_PIN1_GPIO, 0);
            }*/
        }      
    } else {
        if (s.write && is_cm4_watchdog_enabled) {            
            s.success = cancel_repeating_timer(&cm4_watchdog_timer);
            if (s.success) {
                is_cm4_watchdog_enabled = false;                
                // Turn OFF(1) Pico LED1
                //gpio_put(LED_PIN1_GPIO, 1);
            }
        }
    }

    s.enable = is_cm4_watchdog_enabled;
        
    // Pack the Watchdog response message
    pico_pkt_watchdog_resp_pack(b->resp, &s);
    // Write response to host (blocking)
    uart_write_blocking(UART_ID, b->resp, PICO_PKT_LEN);
}