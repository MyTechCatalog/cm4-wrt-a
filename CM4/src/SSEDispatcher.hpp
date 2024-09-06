/**
 * Copyright (c) 2024 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef EVENT_DISPATCHER_HPP
#define EVENT_DISPATCHER_HPP

#include <atomic>
#include <condition_variable>
#include "httplib.h"
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include "Event.hpp"


class SSEDispatcher
{

public:
    SSEDispatcher();
    ~SSEDispatcher();

    void pause();
    void resume();
    void wait_event(httplib::DataSink *sink);
    void send_event(const std::string &message);

private:
    std::mutex m_;
    std::condition_variable cv_;
    std::atomic_int id_;
    std::atomic_int cid_;
    std::string message_;
    Event stopEvent_;
};


#endif