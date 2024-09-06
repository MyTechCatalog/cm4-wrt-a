/**
 * Copyright (c) 2024 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef EVENT_HPP
#define EVENT_HPP
#include <chrono>             // std::chrono::seconds
#include <mutex>              // std::mutex, std::unique_lock
#include <condition_variable> // std::condition_variable, std::cv_status

class Event
{

public:
    Event(/* args */);
    ~Event();
    void set();
    void clear();
    bool isSet();

    /// @brief Waits for the specified relative time to expire before returning, 
    /// unless set was called, in which case, it returns true, false otherwise.
    /// @tparam T1 
    /// @tparam T2 
    /// @param rel_time 
    /// @return Returns true if set, false otherwise.
    template<typename T1, typename T2>
    bool wait(const std::chrono::duration<T1, T2>& rel_time){
        std::unique_lock<std::mutex> lck(mtx_);
        cv_.wait_for(lck, rel_time);
        return flag_;
    }
    
private:
    std::mutex mtx_;
    std::condition_variable cv_;
    bool flag_;
};

#endif //EVENT_HPP