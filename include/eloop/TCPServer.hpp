#pragma once

#include "eloop/TCPServerSingle.hpp"
#include "eloop/callback.hpp"
#include "eloop/channel.hpp"
#include "eloop/eventLoopThread.hpp"
#include "eloop/inetAddress.hpp"
#include "eloop/noncopyable.hpp"
#include <cstddef>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>


namespace eloop
{

class EventLoopThread;
class TCPServerSingle;
class EventLoop;
class InetAddress;


class TCPServer: noncopyable
{
public:
    TCPServer(EventLoop *loop, const InetAddress &local);
    ~TCPServer();
    void setNumThread(size_t n);
    void start();

    void setThreadInitCallback(const ThreadInitCallback &cb)
    {
        threadInitCallback_ = cb;
    }

    void setConnectionCallback(const ConnectionCallback &cb)
    {
        connectionCallback_ = cb;
    }

    void setMessageCallback(const MessageCallback &cb)
    {
        messageCallback_ = cb;
    }

    void setWriteCompleteCallback(const WriteCompleteCallback &cb)
    {
        writeCompleteCallback_ = cb;
    }

private:
    using ThreadPtr = std::unique_ptr<std::thread>;
    using ThreadPtrList = std::vector<ThreadPtr>;
    using TCPServerSinglePtr = std::unique_ptr<TCPServerSingle>;
    using EventLoopList = std::vector<EventLoop*>;

    EventLoop *baseLoop_;
    TCPServerSinglePtr baseServer_;
    ThreadPtrList threads_;
    EventLoopList eventLoops_;
    size_t numThread_;
    std::atomic_bool started_;
    InetAddress local_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback threadInitCallback_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;

    void startInLoop();
    void runInLoop(size_t index);
};

}
