/**
 * Copyright (c) 2024 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */
#include "SSEDispatcher.hpp"

using namespace httplib;
using namespace std;

SSEDispatcher::SSEDispatcher()
:id_{0}
,cid_{-1}
{
}

SSEDispatcher::~SSEDispatcher()
{
}

void SSEDispatcher::wait_event(httplib::DataSink *sink){
    unique_lock<mutex> lk(m_);
    int id = id_;
    if (cv_.wait_for(lk, std::chrono::milliseconds(1), [&] { return cid_ == id; })) {
        if (!stopEvent_.isSet()) {
            sink->write(message_.data(), message_.size());
        }
    } 
}

void SSEDispatcher::send_event(const std::string &message){
    lock_guard<mutex> lk(m_);
    cid_ = id_++;
    message_ = message;
    cv_.notify_all();
}

void SSEDispatcher::pause(){
    stopEvent_.set();
}
void SSEDispatcher::resume(){
    stopEvent_.clear();
}