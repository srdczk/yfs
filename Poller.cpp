#include "Poller.h"
#include <unistd.h>
#include <cstring>
#include <cassert>

PollManager::PollManager(): pendingChange_(false) {

    memset(callbacks_, '\0', sizeof(void *) * MAX_FDS);
    manager_ = new EpollAIO;
    thread_ = std::make_shared<std::thread>(std::bind(&PollManager::Loop, this));

}

PollManager::~PollManager() {
    // never kill
    assert(0);
    if (thread_ && thread_->joinable()) thread_->join();
}

PollManager *PollManager::Instance() {
    // global singleton
    static PollManager manager;
    return &manager;
}

void PollManager::AddCallback(int fd, PollFlag flag, AIOCallback *callback) {
    assert(fd < MAX_FDS);

    std::lock_guard<std::mutex> guard(mutex_);
    manager_->WatchFd(fd, flag);
    assert(!callbacks_[fd] || callbacks_[fd] == callback);
    callbacks_[fd] = callback;
}

void PollManager::RemoveCallback(int fd, PollFlag flag) {
    std::lock_guard<std::mutex> guard(mutex_);
    if (manager_->UnwatchFd(fd, flag)) {
        callbacks_[fd] = nullptr;
    }
}

bool PollManager::HasCallback(int fd, PollFlag flag, AIOCallback *callback) {
    std::lock_guard<std::mutex> guard(mutex_);
    if (!callbacks_[fd] || callbacks_[fd] != callback) {
        return false;
    }
    return manager_->OnWatch(fd, flag);
}

// block remove, never be called again
void PollManager::BlockRemoveFd(int fd) {
    std::unique_lock<std::mutex> lock(mutex_);
    manager_->UnwatchFd(fd, CB_RDWR);
    pendingChange_ = true;
    cv_.wait(lock);
    callbacks_[fd] = nullptr;
}

void PollManager::Loop() {
    while (true) {
        {
            std::lock_guard<std::mutex> guard(mutex_);
            if (pendingChange_) {
                pendingChange_ = false;
                cv_.notify_all();
            }
        }
        readable_.clear();
        writable_.clear();

        manager_->WaitReady(&readable_, &writable_);

        if (readable_.empty() && writable_.empty())
            continue;

        for (auto &fd : readable_) {
            if (fd < MAX_FDS && callbacks_[fd])
                callbacks_[fd]->ReadCallback(fd);
        }

        for (auto &fd : writable_) {
            if (fd < MAX_FDS && callbacks_[fd])
                callbacks_[fd]->WriteCallback(fd);
        }

    }
}


EpollAIO::EpollAIO() {
    // create epoll
    pollFd_ = epoll_create(MAX_FDS);
    assert(pollFd_ > -1);
    memset(status_, '\0', sizeof(int) * MAX_FDS);
}

EpollAIO::~EpollAIO() {
    close(pollFd_);
}

int EpollAIO::FlagToFd(PollFlag flag) {
    int res;
    if (flag == CB_RDONLY) {
        res = EPOLLIN;
    } else if (flag == CB_WRONLY) {
        res = EPOLLOUT;
    } else {
        res = EPOLLIN | EPOLLOUT;
    }
    return res;
}

void EpollAIO::WatchFd(int fd, PollFlag flag) {
    assert(fd < MAX_FDS);

    struct epoll_event ev;
    int op = status_[fd] ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    status_[fd] |= (int)flag;
    // Epoll ET mod
    ev.events = EPOLLET;
    ev.data.fd = fd;

    if (status_[fd] & CB_RDONLY) {
        ev.events |= EPOLLIN;
    } else if (status_[fd] & CB_WRONLY) {
        ev.events |= EPOLLOUT;
    }

    assert(!epoll_ctl(pollFd_, op, fd, &ev));
}

bool EpollAIO::UnwatchFd(int fd, PollFlag flag) {
    assert(fd < MAX_FDS);
    status_[fd] &= ~((int)flag);
    struct epoll_event ev;
    int op = status_[fd] ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

    ev.events = EPOLLET;
    ev.data.fd = fd;

    if (status_[fd] & CB_RDONLY) {
        ev.events |= EPOLLIN;
    } else if (status_[fd] & CB_WRONLY) {
        ev.events |= EPOLLOUT;
    }
    assert(!epoll_ctl(pollFd_, op, fd, &ev));

    return op == EPOLL_CTL_DEL;

}

bool EpollAIO::OnWatch(int fd, PollFlag flag) {
    assert(fd < MAX_FDS);

    return (status_[fd] & CB_MASK) == flag;
}

void EpollAIO::WaitReady(std::vector<int> *readable, std::vector<int> *writable) {

    int num = epoll_wait(pollFd_, ready_, MAX_FDS, -1);

    for (int i = 0; i < num; ++i) {
        if (ready_[i].events & EPOLLIN) {
            readable->push_back(ready_[i].data.fd);
        }
        if (ready_[i].events & EPOLLOUT) {
            writable->push_back(ready_[i].data.fd);
        }
    }

}





