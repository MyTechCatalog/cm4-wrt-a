/**
 * Copyright (c) 2024 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */
#include "Event.hpp"

Event::Event(/* args */)
:flag_(false)
{
}

Event::~Event()
{
}

void Event::set(){
    std::unique_lock<std::mutex> lck(mtx_);
    flag_ = true;
    cv_.notify_all();    
}

void Event::clear(){
    std::unique_lock<std::mutex> lck(mtx_);
    flag_ = false;
}

bool Event::isSet(){
     std::unique_lock<std::mutex> lck(mtx_);
     return flag_;
}