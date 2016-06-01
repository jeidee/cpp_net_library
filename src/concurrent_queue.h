#ifndef __CONCURRENT_QUEUE_H__
#define __CONCURRENT_QUEUE_H__

#include <boost/container/deque.hpp>
#include "auto_lock.h"

template<typename T>
class concurrent_queue {
public:
    concurrent_queue() {
        mtx_ = boost::make_shared<boost::mutex>();
    }
    
    void enqueue(T* data) {
        auto_lock lock(mtx_);
        
        queue_.push_back(data);
    }
    
    T* dequeue() {
        auto_lock lock(mtx_);
        
        if (queue_.size() < 1) {
            return nullptr;
        }
        
        T* data = queue_.front();
        queue_.pop_front();
        return data;
    }
    
private:
    boost::container::deque<T*> queue_;
    boost::shared_ptr<boost::mutex> mtx_;
};

#endif  // __CONCURRENT_QUEUE_H__