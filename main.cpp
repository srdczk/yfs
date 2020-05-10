#include "ThreadPool.h"
#include "iostream"



int main() {
    ThreadPool pool;

    int i = 0;
    std::mutex lock;
    for (int j = 0; j < 100; ++j) {
        pool.AddTask([&]() {
            std::lock_guard<std::mutex> guard(lock);
            std::cout << "NIMASILE:" << i++ << "\n";
        });
    }
    pool.Stop();
//    std::this_thread::sleep_for(std::chrono::seconds(100));

    return 0;
}