#ifndef __CONCURRENT_STACK_H
#define __CONCURRENT_STACK_H

#include <boost/container/deque.hpp>
#include "auto_lock.h"

template<typename T>
class concurrent_stack {
public:
    concurrent_stack() {
        mtx_ = boost::make_shared<boost::mutex>();
    }
    
    void push(T data) {
        auto_lock lock(mtx_);
        
        stack_.push_back(data);
    }
    
    T pop() {
        auto_lock lock(mtx_);
        
        if (stack_.size() < 1) {
            return nullptr;
        }
        
        T data = stack_.back();
        stack_.pop_back();
        return data;
    }
    
private:
    boost::container::deque<T> stack_;
    boost::shared_ptr<boost::mutex> mtx_;
};

#endif  // __CONCURRENT_STACK_H