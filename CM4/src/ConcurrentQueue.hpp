
/**
 * Copyright (c) 2024 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */
#if !defined(CONCURRENT_QUEUE_HPP)
#define CONCURRENT_QUEUE_HPP

#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>

template<typename Data>
class ConcurrentQueue
{
public:
    ConcurrentQueue(size_t max_size = 10)
	:max_queue_size(max_size){}

    void push(Data const& data) {
        std::unique_lock lock(the_mutex);
		if (the_queue.size() < max_queue_size)
		{
			the_queue.push(data);
			lock.unlock();
			the_condition_variable.notify_one();
		}
		else
		{
			lock.unlock();
		}
    }

    bool empty() const {
		std::scoped_lock lock(the_mutex);
        return the_queue.empty();
    }

	bool isFull() const {
		std::scoped_lock lock(the_mutex);
		return (the_queue.size() == max_queue_size);
	}

    size_t size() const {
		std::scoped_lock lock(the_mutex);
        return the_queue.size();
    }

    bool try_pop(Data& popped_value) {
		std::scoped_lock lock(the_mutex);
        if(the_queue.empty())
        {
            return false;
        }

        popped_value=the_queue.front();
        the_queue.pop();
        return true;
    }

    void wait_and_pop(Data& popped_value) {
		std::unique_lock lock(the_mutex);
        
        while(the_queue.empty()) {
            the_condition_variable.wait(lock);
        }

        popped_value=the_queue.front();
        the_queue.pop();
    }

    /// @brief Waits for the specified relative time to expire before returning, 
    /// unless push was called, in which case, it returns true, false otherwise.
    /// @param popped_value
    /// @tparam T1 
    /// @tparam T2 
    /// @param rel_time 
    /// @return Returns true if set, false otherwise.
    template<typename T1, typename T2>
    bool wait_and_pop(Data& popped_value, const std::chrono::duration<T1, T2>& rel_time){
        std::unique_lock lock(the_mutex);
        if (the_queue.empty()){
            the_condition_variable.wait_for(lock, rel_time);
        }
        
        bool success = !the_queue.empty();
        
        if (success) {
            popped_value = the_queue.front();
            the_queue.pop();
        }

        return success;
    }
    
private:
    // Disallow copying the object
    void operator=(const ConcurrentQueue &){}
    ConcurrentQueue(const ConcurrentQueue &){}
	size_t max_queue_size;
    std::queue<Data> the_queue;
    mutable std::mutex the_mutex;
	std::condition_variable the_condition_variable;
};

#endif // #if !defined(CONCURRENT_QUEUE_HPP)

