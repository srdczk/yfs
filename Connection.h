#ifndef RPC_CONNECTION_H
#define RPC_CONNECTION_H

#include "Poller.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unordered_map>


class Connection;

class ChanManager {
public:
    virtual bool GotPdu(Connection *connection, char *buff, int size) = 0;
    virtual ~ChanManager() = default;
};

class Connection : public AIOCallback {

public:
    struct Buffer {
        Buffer(): buff_(nullptr), size_(0), bytes_(0) {}
        Buffer(char *buff, int size): buff_(buff), size_(size), bytes_(0) {}
        char *buff_;
        int size_;
        // amount of bytes written or read so far
        int bytes_;
    };

    Connection(ChanManager *manager, int f, int lossyTest = 0);
    ~Connection();

    int ChanNo();
    bool IsDead();
    void CloseConn();

    bool Send(char *buff, int size);
    void WriteCallback(int fd) override;
    void ReadCallback(int fd) override;

    int IncRef();
    int DecRef();

    int Ref();

    int Compare(Connection *);

private:
    bool ReadPdu();
    bool WritePdu();

private:
    ChanManager *manager_;
    int fd_;
    std::atomic_bool dead_;
    Buffer write_;
    Buffer read_;
    struct timeval createTime_;
    int waiters_;
    int refNo_;
    int lossy_;

    std::mutex mutex_;
    std::mutex refMutex_;

    std::condition_variable sendComplete_;
    std::condition_variable sendWait_;

};

#endif //RPC_CONNECTION_H
