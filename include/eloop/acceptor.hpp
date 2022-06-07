#pragma once

#include <memory>
#include "eloop/noncopyable.hpp"
#include "eloop/inetAddress.hpp"
#include "eloop/channel.hpp"
#include "eloop/callback.hpp"

namespace eloop
{

class EventLoop;

class Acceptor: noncopyable
{
public:
    Acceptor(EventLoop *loop, const InetAddress &local);
    ~Acceptor();

    bool listening() const
    {
        return listening_;
    }

    void setNewConnectionCallback(const NewConnectionCallback &cb)
    {
        newConnectionCallback_ = cb;
    }

    void listen();
private:
    bool listening_;
    EventLoop *loop_;
    const int acceptFd_;
    Channel acceptChannel_;
    InetAddress local_;
    NewConnectionCallback newConnectionCallback_;

    void handleRead();
};

}
