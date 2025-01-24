
/**
 * Copyright (c) 2024 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */
#include "PacketHandler.hpp"
#include "syshead.h"
#include "Utils.hpp"
#include "pico_pkt_temperature.h"
#include "pico_pkt_ping.h"
#include "pico_pkt_fan_pwm.h"
#include "pico_pkt_watchdog.h"
#include "pico_pkt_shutdown.h"

PacketHandler::PacketHandler()
:pico_fd_{0}
,pktQueues_{ {PICO_PKT_PING_MAGIC, std::make_shared<ConcurrentQueue<BufPtr>>(10)},
    {PICO_PKT_TEMPERATURE_MAGIC, std::make_shared<ConcurrentQueue<BufPtr>>(10)},
    {PICO_PKT_FAN_PWM_MAGIC, std::make_shared<ConcurrentQueue<BufPtr>>(10)},
    {PICO_PKT_WATCHDOG_MAGIC, std::make_shared<ConcurrentQueue<BufPtr>>(10)},
    {PICO_PKT_VERSION_MAGIC, std::make_shared<ConcurrentQueue<BufPtr>>(10)} }
{
}

PacketHandler::~PacketHandler()
{
  close(pico_fd_);
}

PacketHandler& PacketHandler::instance()
{
    static PacketHandler theInstance;
    return theInstance;
}

bool PacketHandler::is_init_done(){
    return initDone_.wait(std::chrono::seconds(3));
}

int PacketHandler::run(){
    int retVal  = EXIT_SUCCESS;
    int result = 0;
    int maxfd; // maximum file descriptor used
    fd_set readfds; // file descriptor set
    
    // open the device to be non-blocking (read will return immediately)
    pico_fd_ = open(appSettings.pico_serial_device_path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (pico_fd_ < 0) {       
        print_err("Failed to open: %s, %s\n", 
            appSettings.pico_serial_device_path.c_str(), strerror(errno));
        retVal  = EXIT_FAILURE; 
        goto cleanup; 
    }

    // Put the serial port into exclusive mode.
    result = ioctl(pico_fd_, TIOCEXCL);
    if (result == -1) {
        print_err("Failed to put the serial port %s into exclusive mode: %s\n",
            appSettings.pico_serial_device_path.c_str(), strerror(errno));
        retVal  = EXIT_FAILURE;
        goto cleanup;
    }

    // Try and place an exclusive advisory lock on the open device
    result = flock(pico_fd_, LOCK_EX | LOCK_NB);
    if (result == EWOULDBLOCK) {
        print_err("Another process has already opened serial port %s\n", appSettings.pico_serial_device_path.c_str());
        retVal  = EXIT_FAILURE; 
        goto cleanup;
    }

    struct termios settings;    
    get_serial_port_settings(settings);
    
    result = tcflush(pico_fd_, TCIFLUSH);
    if (result == -1) {
        print_err("Failed to flush serial port: %s\n", strerror(errno));
    }
    
    result = tcsetattr(pico_fd_, TCSANOW, &settings);
    if (result == -1) {
        print_err("Failed to apply serial port settings: %s\n", strerror(errno));
        retVal  = EXIT_FAILURE; 
        goto cleanup;
    }

    initDone_.set();
    
    maxfd = pico_fd_ + 1;  // maximum bit entry (pico_fd_) to test

    while (!getQuitEvent().isSet()) {
        // clear the socket set 
        FD_ZERO(&readfds);
        FD_SET(pico_fd_, &readfds);
        
        struct timeval timeout;
        // set timeout value
        timeout.tv_usec = 0;  // Microseconds
        timeout.tv_sec  = 1;  // Seconds
        int res = select(maxfd, &readfds, NULL, NULL, &timeout);
        if (res == -1) {
            // Handle error.
            print_err("select() failed on serial port: %s\n", strerror(errno));
        } else if ((res > 0) && (FD_ISSET(pico_fd_, &readfds))) {
            // message from pico to host is available
            handle_message_from_pico();
        }
        
    }//@END while (true)

cleanup:    
    close(pico_fd_);
    pico_fd_ = 0;   
    return retVal; 
}

void PacketHandler::handle_message_from_pico() {    
    static pkt_buf pkt = {0,0,0};
    static bool initDone = false;
       
    static bool have_request = false;

    if (!initDone) {
        initDone = true;

        memset((void *)&pkt.req, 0, sizeof(pkt.req));
        memset((void *)&pkt.resp, 0, sizeof(pkt.resp));
        pkt.ready = false;
    }

    static size_t numBytesRead = 0;
    static size_t numBytesToRead = PICO_PKT_LEN;

    int res = read(pico_fd_, (void*)&pkt.resp[numBytesRead], numBytesToRead);
            
    if (res == -1) {
        print_err("Host failed to read serial port: %s\n", strerror(errno));
        return;
    }

    numBytesRead += res;
    numBytesToRead -= res;

    // Bookkeeping: reset counters as necessary
    if (numBytesRead == PICO_PKT_LEN) {        
        have_request = true;
        numBytesRead = 0;
        numBytesToRead = PICO_PKT_LEN;
    }

    if (!have_request) {
        return;
    }

    // We have a command in the UART

    // Pointer to currently active packet handler    
    const uint8_t *magic = &pkt.resp[PKT_MAGIC_IDX];
    
    pkt.ready = false;
    have_request = false;

    // Determine which packet queue should receive this message
    if (auto search = pktQueues_.find(*magic); search != pktQueues_.end()) {
        auto pktQ = search->second.get();
        BufPtr ptr(new uint8_t[PICO_PKT_LEN]);
        if (pktQ && !pktQ->isFull() && ptr.get()) {
            memcpy(ptr.get(), &pkt.resp, PICO_PKT_LEN);
            pktQ->push(ptr);
        }       
    } else if (*magic == PICO_PKT_SHUTDOWN_MAGIC) {
        pkt_shutdown(&pkt);
    } else {
        // We are out of sync, just discard the data and
        // wait for the next packet
        print_err("Host out of sync.\n")
        print_bytes("Response data:", pkt.resp, PICO_PKT_LEN);
    } 
}

ssize_t PacketHandler::send_pico_request(const uint8_t * buf, size_t length){
    std::lock_guard<std::mutex> lk(m_);
    
    int result = tcflush(pico_fd_, TCIFLUSH);
    if (result == -1) {
        print_err("Failed to flush serial port: %s\n", strerror(errno));
    }
       
    ssize_t bytesWritten = write(pico_fd_, buf, length);
    
    if (bytesWritten != (ssize_t)length) {
       print_err("Error writing to serial port: %s\n", strerror(errno));
    }

    return bytesWritten;
}

bool PacketHandler::get_pico_response(pkt_buf &pkt, uint8_t pkt_id) {
    bool success = false;

    if (auto search = pktQueues_.find(pkt_id); search != pktQueues_.end()) {
        auto pktQ = search->second.get();
        BufPtr ptr;
        if (pktQ && pktQ->wait_and_pop(ptr, std::chrono::seconds(1))) {
            memcpy(&pkt.resp, ptr.get(), PICO_PKT_LEN);
            success = true;
        }  
    }

    return success;
}