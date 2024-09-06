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
bool is_cm4_graceful_shutdown = false;

void pkt_shutdown(struct pkt_buf *b){
    is_cm4_graceful_shutdown = true;
}

void reset_the_cm4() {
    // In case the power rails were off due to a previous graceful shutdown:
    // Turn on LED power rail (LED_V+) 
    gpio_put(LED_PWR_GPIO, 1);

    // Turn on the PCIe switch
    gpio_put(PCIE_SWITCH_PWR_EN_GPIO, 1);

    // Turn on the M.2 Sockets and Ethernet1
    gpio_put(M2_PWR_EN_GPIO, 1);

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
        if (!gpio_get(CM4_RST_GPIO)) {
             // The CM4 is off. Turn it on.
            reset_the_cm4();
        } else if ((buttonUpTime > buttonDownTime) && 
            (diff >= GRACEFUL_SHUTDOWN_MIN_THRESH_uSEC) &&
            (diff < HARD_RESET_MIN_THRESH_uSEC)) {
                is_host_shutdown_request_pending = true;
        } else if ((buttonUpTime > buttonDownTime) && 
            (diff >= HARD_RESET_MIN_THRESH_uSEC)) {
            is_host_hard_reset_request_pending = true;
        }
    }
}

void handle_cm4_events(uint32_t events){
    // This function is called by an interrupt service routing
    // Do not do anything that holds up the processor, like
    // print statements, blocking I/O etc.
    if (events & GPIO_IRQ_EDGE_FALL) {
        if (is_cm4_graceful_shutdown) {
            // Turn OFF(1) Pico LED1
            //gpio_put(LED_PIN1_GPIO, 1);

            // Turn off LED power rail (LED_V+) 
            gpio_put(LED_PWR_GPIO, 0);

            // Turn off the PCIe switch
            gpio_put(PCIE_SWITCH_PWR_EN_GPIO, 0);

            // Turn off the M.2 Sockets and Ethernet1
            gpio_put(M2_PWR_EN_GPIO, 0);
        }
    } else if (events & GPIO_IRQ_EDGE_RISE) {
        is_cm4_graceful_shutdown = false;
        // Turn ON(0) Pico LED1
        //gpio_put(LED_PIN1_GPIO, 0);
    }
}