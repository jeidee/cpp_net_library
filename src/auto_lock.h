#ifndef __AUTO_LOCK_H__
#define __AUTO_LOCK_H__

#include <boost/thread/mutex.hpp>


class auto_lock {
public:
    auto_lock(boost::shared_ptr<boost::mutex> mtx) : mtx_(mtx) {
        mtx_->lock();
    }
    
    ~auto_lock() {
        mtx_->unlock();
    }
    
private:
    boost::shared_ptr<boost::mutex> mtx_;
};

#endif  // __AUTO_LOCK_H__