#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <stdexcept>
#include <utility>

template<typename T>
class FifoQueue {
public:
    FifoQueue() = default;

    void push(T value) {
        mutex_.lock();
        queue_.push(value);
        mutex_.unlock();
    }

    T pop() {
        mutex_.lock();
        T value = std::move(queue_.front());
        queue_.pop();
        mutex_.unlock();
        return value;
    }

    std::size_t size() {
        mutex_.lock();
        size_t ret = queue_.size();
        mutex_.unlock();
        return ret;
    }

private:
    std::mutex mutex_;
    std::queue<T> queue_;
};