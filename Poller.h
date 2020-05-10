#ifndef RPC_POLLER_H
#define RPC_POLLER_H

#include <sys/epoll.h>
#include <atomic>
#include <vector>
#include <mutex>
#include <memory>
#include <condition_variable>
#include <thread>
#include <functional>

#define MAX_FDS 128

typedef enum {
    CB_NONE = 0x0,
    CB_RDONLY = 0x1,
    CB_WRONLY = 0x10,
    CB_RDWR = 0x11,
    CB_MASK = ~0x11,
} PollFlag;

class AIOManager {
public:
    virtual void WatchFd(int fd, PollFlag flag) = 0;
    virtual bool UnwatchFd(int fd, PollFlag flag) = 0;
    virtual bool OnWatch(int fd, PollFlag flag) = 0;
    virtual void WaitReady(std::vector<int> *readable, std::vector<int> *writable) = 0;
    virtual ~AIOManager() = default;
};

class AIOCallback {
public:
    virtual void ReadCallback(int fd) = 0;
    virtual void WriteCallback(int fd) = 0;
    virtual ~AIOCallback() = default;
};

class PollManager {
public:
    // Not Copyable
    PollManager(const PollManager &) = delete;
    PollManager &operator=(const PollManager &) = delete;

    ~PollManager();
    // Singleton
    static PollManager *Instance();

    void AddCallback(int fd, PollFlag flag, AIOCallback *callback);
    void RemoveCallback(int fd, PollFlag flag);
    bool HasCallback(int fd, PollFlag flag, AIOCallback *callback);
    void BlockRemoveFd(int fd);
    void Loop();

private:
    PollManager();

private:
    std::atomic_bool pendingChange_;
    std::shared_ptr<std::thread> thread_;
    std::mutex mutex_;
    std::condition_variable cv_;

    AIOCallback *callbacks_[MAX_FDS];
    AIOManager *manager_;

    std::vector<int> readable_;
    std::vector<int> writable_;
};

class EpollAIO : public AIOManager {
public:
    EpollAIO();
    ~EpollAIO() override;
    void WatchFd(int fd, PollFlag flag) override;
    bool UnwatchFd(int fd, PollFlag flag) override;
    bool OnWatch(int fd, PollFlag flag) override;
    void WaitReady(std::vector<int> *readable, std::vector<int> *writable) override;

private:
    static int FlagToFd(PollFlag flag);
private:
    int pollFd_;
    struct epoll_event ready_[MAX_FDS];
    int status_[MAX_FDS];
};


#endif //RPC_POLLER_H
