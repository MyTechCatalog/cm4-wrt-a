/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#include "pico/stdlib.h"

#ifndef CM4_WRT_A_H_
#define CM4_WRT_A_H_

// LED_V+ Power
#define LED_PWR_GPIO 3 // GPIO3

// Pico LED1
#define LED_PIN1_GPIO 25 // GPIO25
    
// Pico LED2
#define LED_PIN2_GPIO 24 // GPIO24
    
// Pico LED3
#define LED_PIN3_GPIO 23 // GPIO23
    
// Pico LED4
#define LED_PIN4_GPIO 22 // GPIO22
    
// Power enable for the M.2 sockets and ETH1
#define M2_PWR_EN_GPIO 12 // GPIO12
    
// PCIe switch power enable
#define PCIE_SWITCH_PWR_EN_GPIO 13 // GPIO13

// CM4 Run (RUN_PG)
#define CM4_RUN_GPIO 16 // GPIO16

// CM4 External Reset (nEXTRST)
#define CM4_RST_GPIO 17 // GPIO17

// CM4 Boot (nRPIBOOT)
#define CM4_BOOT_GPIO 18 // GPIO18

// CM4 Global Enable (GLOBAL_EN)
#define CM4_EN_GPIO 19 // GPIO19

// Main system fan's tachometer pin
#define FAN1_TACHO_GPIO 9 // GPIO9

// Main system fan's PWM pin
#define FAN1_PWM_GPIO 8 // GPIO8

// CM4 fan tachometer pin
#define CM4_FAN_TACHO_GPIO 11 // GPIO11

// CM4 fan PWM pin
#define CM4_FAN_PWM_GPIO 10 // GPIO10

// Input pin connected to shutdown button
// Pin 3 on jumper with reference J16
#define SHUTDOWN_REQUEST_GPIO 7 // GPIO7


#define UART_ID uart0
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 0
#define UART_RX_PIN 1

// Minimum length of time (uS) that the shutdown button
// has to be pressed to trigger a shutdown request 
// message to be sent to the Host (CM4)
#define GRACEFUL_SHUTDOWN_MIN_THRESH_uSEC 250000LL

// Minimum length of time (uS) that the shutdown button
// has to be pressed to trigger a hard reset of the Host (CM4)
#define HARD_RESET_MIN_THRESH_uSEC 3000000LL

// Returns 64 bit time from the timer
uint64_t get_time(void);

#endif