/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#include <string.h>
#include "pico_pkt_shutdown.h"
#include "cm4-wrt-a.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"


/// @brief System time when the shutdown button is pressed
uint64_t buttonDownTime = 0;
/// @brief System time when the shutdown button is released
uint64_t buttonUpTime = 0;

extern bool is_host_shutdown_request_pending;
extern bool is_host_hard_reset_request_pending;

void pkt_shutdown(struct pkt_buf *b){
    // The Pico does not do anything with this request from the host.    
}

void reset_the_cm4() {
    uint saved_pin_dir = gpio_get_dir(CM4_RUN_GPIO);
    gpio_set_dir(CM4_RUN_GPIO, GPIO_OUT);
    gpio_put(CM4_RUN_GPIO, 0);
    sleep_us(1);
    gpio_put(CM4_RUN_GPIO, 1);
    // Restore the original pin direction
    gpio_set_dir(CM4_RUN_GPIO, saved_pin_dir);
}

void send_shutdown_request(){
    pkt_buf pkt = {0};
    bool success = true;
    memset((void *)&pkt.resp, 0, sizeof(pkt.resp));
    // Note the use of the pkt.resp buffer here because that is where
    // the host places incoming UART commands.
    pico_pkt_shutdown_resp_pack((uint8_t *)pkt.resp, success);
    // Write response to host (blocking)        
    uart_write_blocking(UART_ID, pkt.resp, PICO_PKT_LEN);
}

void detect_shutdown_events(uint32_t events){
    // This function is called by an interrupt service routing
    // Do not do anything that holds up the processor, like
    // print statements, blocking I/O etc.
    if (events & GPIO_IRQ_EDGE_FALL) {
        // Get time of falling edge
        buttonDownTime = get_time();
        buttonUpTime = buttonDownTime;
    } else if (events & GPIO_IRQ_EDGE_RISE) {
        // Get time of rising edge
        buttonUpTime = get_time();
        uint64_t diff = buttonUpTime - buttonDownTime;
        if ((buttonUpTime > buttonDownTime) && 
            (diff >= GRACEFUL_SHUTDOWN_MIN_THRESH_uSEC) &&
            (diff < HARD_RESET_MIN_THRESH_uSEC)) {
                is_host_shutdown_request_pending = true;
        } else if ((buttonUpTime > buttonDownTime) && 
            (diff >= HARD_RESET_MIN_THRESH_uSEC)) {
            is_host_hard_reset_request_pending = true;
        }
    }
}