#include "ThreadPool.h"

ThreadPool::ThreadPool(int threadNum): threadNum_(threadNum), running_(true), queue_(100 * threadNum_) {
    Start();
}

ThreadPool::~ThreadPool() { Stop(); }

void ThreadPool::AddTask(ThreadPool::Task task) {
    queue_.Add(task, true);
}

void ThreadPool::Start() {

    for (int i = 0; i < threadNum_; ++i) {
        threads_.push_back(std::make_shared<std::thread>(std::bind(&ThreadPool::ThreadFunc, this)));
    }

}

void ThreadPool::Stop() {
    std::call_once(flag_, [this]() {
        running_ = false;
        for (auto &thread : threads_) {
            if (thread && thread->joinable())
                thread->join();
        }
        threads_.clear();
    });
}

void ThreadPool::ThreadFunc() {
    while (running_) {
        Task task;
        queue_.Take(&task);
        task();
    }
}






