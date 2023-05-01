/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#include "cm4-wrt-a.h"
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "hardware/adc.h"
#include "hardware/irq.h"
#include "pico_pkt_temperature.h"
#include "pico_pkt_ping.h"
#include "pico_pkt_fan_pwm.h"
#include "pico_pkt_watchdog.h"
#include "pico_pkt_shutdown.h"
#include "pico_pkt_version.h"

#define SET_PIN_DIR(gpio, direction)\
    gpio_init(gpio);\
    gpio_set_dir(gpio, direction);

/*! @brief Turn on power to PCIe switch, M.2 Sockets and Ethernet1.
*
*   This function initializes the board.
*/
static inline void init_board();

static void init_uart();
static void uart_write_response(uint8_t *resp);

/*! @brief  Check whether GPIO04 and GPIO05 pins are shorted
*
* This fucntion checks whether GPIO04 and GPIO05 pins are shorted
* thereby indicating the user's desire to place the CM4 into
* USB boot mode. 
* \return True if GPIO04 and GPIO05 pins are shorted, false otherwise.
*/
static inline bool is_CM4_USB_boot_mode_enabled();

static bool is_CM4_in_USB_Boot_mode = false;

// Select correct interrupt for the UART we are using
static int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

volatile bool is_host_shutdown_request_pending = false;
volatile bool is_host_hard_reset_request_pending = false;

struct pkt_buf pkt;
static const struct pkt_handler pkt_handlers[] = {
    PKT_PING,
    PKT_TEMPERATURE,
    PKT_FAN_PWM,
    PKT_WATCHDOG,
    PKT_SHUTDOWN,
    PKT_VERSION
};

static inline void blink_led(uint gpio, uint numTimes) {
    
    for (size_t i = 0; i < numTimes; i++) {
        gpio_put(gpio, 0);
        sleep_ms(250);
        gpio_put(gpio, 1);
        sleep_ms(250);
    }
}

// GPIO interrupt handler
void gpio_isr_callback(uint gpio, uint32_t events) {
    if (gpio == SHUTDOWN_REQUEST_GPIO) {
        detect_shutdown_events(events);
    } else {
        update_tachometer_counter(gpio, events);
    }
}

int main() {
    
    // Pointer to currently active packet handler
    const struct pkt_handler *handler;

    // Marked volatile to ensure we actually read the byte populated by
    // the UART ISR
    const volatile uint8_t *magic = &pkt.req[PKT_MAGIC_IDX];

    volatile bool have_request = false;

    memset(&pkt, 0, sizeof(pkt));
    pkt.ready = false;
    
    // Initialize packet handlers
    for (int i = 0; i < ARRAY_SIZE(pkt_handlers); i++) {
        if (pkt_handlers[i].init != NULL) {
            pkt_handlers[i].init();
        }
    }
    
    init_board();
    init_uart();
        
    while (true) {

        if (is_host_shutdown_request_pending) {
            send_shutdown_request();
            is_host_shutdown_request_pending = false;
        } else if (is_host_hard_reset_request_pending) {
            reset_the_cm4();
            is_host_hard_reset_request_pending = false;
        }               
        
        have_request = (pkt.ready == true);

        if (!have_request) {
            tight_loop_contents();
            continue;
        }

        // A command has been received via the UART
        pkt.ready = false;          
        handler = NULL;

        // Determine which packet handler should receive this message
        for (int i = 0; i < ARRAY_SIZE(pkt_handlers); i++) {            
            if (pkt_handlers[i].magic == *magic) {
                handler = &pkt_handlers[i];
                break;
            }
        }

        if (handler == NULL) {
            // We are out of sync, just discard the data and
            // wait for the next packet
            memset(&pkt, 0, sizeof(pkt));
            pkt.ready = false;
            blink_led(LED_PIN4_GPIO, 4);
            continue;
        }           

        // Process data and execute requested actions
        handler->exec(&pkt);
        
    }//@END while (true)
}

static inline void init_board() {
    
    SET_PIN_DIR(LED_PWR_GPIO, GPIO_OUT)
    SET_PIN_DIR(LED_PIN1_GPIO, GPIO_OUT)
    SET_PIN_DIR(LED_PIN2_GPIO, GPIO_OUT)
    SET_PIN_DIR(LED_PIN3_GPIO, GPIO_OUT)
    SET_PIN_DIR(LED_PIN4_GPIO, GPIO_OUT)
    SET_PIN_DIR(M2_PWR_EN_GPIO, GPIO_OUT)
    SET_PIN_DIR(PCIE_SWITCH_PWR_EN_GPIO, GPIO_OUT)
    SET_PIN_DIR(CM4_RUN_GPIO, GPIO_IN)
    SET_PIN_DIR(CM4_RST_GPIO, GPIO_IN)
    SET_PIN_DIR(CM4_BOOT_GPIO, GPIO_OUT)
    SET_PIN_DIR(CM4_EN_GPIO, GPIO_OUT)
    SET_PIN_DIR(FAN1_TACHO_GPIO, GPIO_IN)
    SET_PIN_DIR(CM4_FAN_TACHO_GPIO, GPIO_IN)
    SET_PIN_DIR(SHUTDOWN_REQUEST_GPIO, GPIO_IN)
    gpio_pull_up(FAN1_TACHO_GPIO);
    gpio_pull_up(CM4_FAN_TACHO_GPIO);
        

    // Turn off Pico LEDs
    gpio_put(LED_PIN1_GPIO, 1);
    gpio_put(LED_PIN2_GPIO, 1);
    gpio_put(LED_PIN3_GPIO, 1);
    gpio_put(LED_PIN4_GPIO, 1);

    // Turn on LED power rail (LED_V+) 
    gpio_put(LED_PWR_GPIO, 1);

    // Turn on the PCIe switch
    gpio_put(PCIE_SWITCH_PWR_EN_GPIO, 1);

    // Turn on the M.2 Sockets and Ethernet1
    gpio_put(M2_PWR_EN_GPIO, 1);

    is_CM4_in_USB_Boot_mode = is_CM4_USB_boot_mode_enabled();
    
    // Turn on the Raspberry Pi Compute Module 4 (CM4)
    if (is_CM4_in_USB_Boot_mode) {
        gpio_put(CM4_BOOT_GPIO, 0);
    } else {
        gpio_put(CM4_BOOT_GPIO, 1);

        gpio_pull_up(SHUTDOWN_REQUEST_GPIO);
        // Setup interrupt handler detect shutdown button presses
        gpio_set_irq_enabled_with_callback(SHUTDOWN_REQUEST_GPIO, 
            GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_isr_callback);
    }

    gpio_put(CM4_EN_GPIO, 1);

    // Init GPIO for analogue use: 
    // Hi-Z, no pulls, disable digital input buffer.
    adc_init();
    adc_set_temp_sensor_enabled(true);

    int ch_mask=0;

    for (int ch=0; ch<NUM_NTC_SENSORS; ch++) {
        adc_gpio_init(26 + ch);
        ch_mask += 1 << ch;
    }

    // Setup interrupt handler to count FAN1's revolutions
    gpio_set_irq_enabled_with_callback(FAN1_TACHO_GPIO, 
        GPIO_IRQ_EDGE_RISE, true, &gpio_isr_callback);

    gpio_set_irq_enabled_with_callback(CM4_FAN_TACHO_GPIO, 
        GPIO_IRQ_EDGE_RISE, true, &gpio_isr_callback);
    
    init_fan_pwm();    
}

static inline bool is_CM4_USB_boot_mode_enabled() {    
    SET_PIN_DIR(4, GPIO_OUT)
    SET_PIN_DIR(5, GPIO_IN)
    gpio_put(4, 1);
    sleep_us(1);
    bool result = gpio_get(5);
    // Clear the pin after checking it.
    gpio_put(4, 0);
    return result;
}

static int rx_idx = 0;

// RX interrupt handler
static void on_uart_rx() {
    uint8_t *req = (uint8_t *)pkt.req;

    while (uart_is_readable(UART_ID)) {

        int ch = uart_getc(UART_ID);

        // Drop the NULL byte at the beginning.
        if((ch == 0) && (rx_idx == 0)) {
            continue;            
        } else {
            req[rx_idx] = ch;
        }
        
        rx_idx++;

        if (rx_idx == PICO_PKT_LEN) {           
            // Tell the main loop that there is a request pending
            pkt.ready = true;                        
            rx_idx = 0;
            update_watchdog();
            break;
        }
    }
}

static void init_uart()
{
    // Set up our UART with a basic baud rate.
    uart_init(UART_ID, 2400);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // Actually, we want a different speed
    // The call will return the actual baud rate selected, which will be as close as
    // possible tinto that requested
    int __unused actual = uart_set_baudrate(UART_ID, BAUD_RATE);

    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(UART_ID, false, false);

    // Set our data format
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(UART_ID, false);

    // Set up a RX interrupt
    // We need to set up the handler first
    // Select correct interrupt for the UART we are using
    //int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

    // Set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART0_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(UART_ID, true, false);

    // Setup complete, now run a loop and wait for RX interrupts
}

uint64_t get_time(void) {
    // Reading low latches the high value
    uint32_t lo = timer_hw->timelr;
    uint32_t hi = timer_hw->timehr;
    return ((uint64_t) hi << 32u) | lo;
}
