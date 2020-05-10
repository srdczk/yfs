#ifndef RPC_THREADPOOL_H
#define RPC_THREADPOOL_H

#include <atomic>
#include <memory>
#include <thread>
#include <vector>
#include <functional>
#include "SyncQueue.h"


class ThreadPool {

public:
    // Not Copyable
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    typedef std::function<void()> Task;

    // let threadNum equal
    ThreadPool(int threadNum = std::thread::hardware_concurrency());
    ~ThreadPool();

    void AddTask(Task task);

    void Start();
    void Stop();
private:
    void ThreadFunc();

private:
    int threadNum_;
    std::atomic_bool running_;
    SyncQueue<Task> queue_;
    std::vector<std::shared_ptr<std::thread>> threads_;
    std::once_flag flag_;
};

#endif //RPC_THREADPOOL_H
