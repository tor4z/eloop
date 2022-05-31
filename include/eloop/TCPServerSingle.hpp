#pragma once

#include <unordered_set>
#include "eloop/callback.hpp"
#include "eloop/acceptor.hpp"
#include "eloop/noncopyable.hpp"

namespace eloop
{

class EventLoop;

class TCPServerSingle: noncopyable
{
public:
    TCPServerSingle(EventLoop *loop, const InetAddress &local);

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

    void start();
private:
    using ConnectionSet = std::unordered_set<TCPConnectionPtr>;

    EventLoop *loop_;
    Acceptor acceptor_;
    ConnectionSet connections_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;

    void newConnection(int connfd, const InetAddress &local, const InetAddress &peer);
    void closeConnection(const TCPConnectionPtr &conn);
};

}
