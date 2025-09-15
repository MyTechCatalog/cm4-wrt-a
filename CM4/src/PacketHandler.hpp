/**
 * Copyright (c) 2024 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef PACKET_HANDLER_HPP
#define PACKET_HANDLER_HPP
#include <sys/types.h>
#include "ConcurrentQueue.hpp"
#include "pkt_handler.h"
#include "Event.hpp"
#include <map>

class PacketHandler
{    
public:
    typedef std::shared_ptr<uint8_t[]> BufPtr;

    static PacketHandler& instance();
    ~PacketHandler();
    PacketHandler(PacketHandler const&)   = delete;
    void operator=(PacketHandler const&)  = delete;
    int run();
    bool is_init_done();

    /// @brief Sends command packet to RPi Pico 
    /// @param buf Packet to sent.
    /// @param length Packet length in bytes.
    /// @return Number of bytes sent.
    ssize_t send_pico_request(const uint8_t * buf, size_t length);

    /// @brief Reads Pico serial port response
    /// @param pkt Received packet
    /// @param pkt_id Expected Packet ID
    /// @return True(1) on success. False(0) on failure.
    bool get_pico_response(pkt_buf &pkt, uint8_t pkt_id);
private:
    int pico_fd_;
    std::mutex m_;
    Event initDone_;
    std::map<uint8_t, std::shared_ptr<ConcurrentQueue<BufPtr>>> pktQueues_;
    PacketHandler();
    void handle_message_from_pico();
        
};

#endif // @END PACKET_HANDLER_HPP