#ifndef RPC_SYNCQUEUE_H
#define RPC_SYNCQUEUE_H

#include <list>
#include <mutex>
#include <condition_variable>

template <typename T>
class SyncQueue {
public:
    SyncQueue(int capacity = 0);
    ~SyncQueue() = default;
    bool Add(T, bool block = true);
    void Take(T *);
    int Size();
private:
    int capacity_;
    std::list<T> queue_;
    std::mutex mutex_;
    std::condition_variable notEmpty_;
    std::condition_variable notFull_;
};

template<typename T>
SyncQueue<T>::SyncQueue(int capacity): capacity_(capacity) {}

template<typename T>
bool SyncQueue<T>::Add(T t, bool block) {
    std::unique_lock<std::mutex> lock(mutex_);

    while (queue_.size() >= capacity_) {
        if (!block) return false;
        notFull_.wait(lock);
    }
    queue_.push_back(t);
    notEmpty_.notify_one();
    return true;
}



template<typename T>
int SyncQueue<T>::Size() {
    std::lock_guard<std::mutex> guard(mutex_);
    return static_cast<int>(queue_.size());
}

template<typename T>
void SyncQueue<T>::Take(T *t) {
    std::unique_lock<std::mutex> lock(mutex_);
    while (queue_.empty()) {
        notEmpty_.wait(lock);
    }
    *t = queue_.front();
    queue_.pop_front();
    notFull_.notify_one();
}


#endif //RPC_SYNCQUEUE_H
