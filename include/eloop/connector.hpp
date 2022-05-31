#pragma once

#include <functional>
#include "eloop/inetAddress.hpp"
#include "eloop/channel.hpp"
#include "eloop/callback.hpp"
#include "eloop/noncopyable.hpp"


namespace eloop
{

class EventLoop;
class InetAddress;

class Connector: noncopyable
{
public:
    Connector(EventLoop *loop, const InetAddress &peer);
    ~Connector();

    void start();
    
    void setNewConnectionCallback(const NewConnectionCallback &cb)
    {
        newConnectionCallback_ = cb;
    }

    void setErrorCallback(const ErrorCallback &cb)
    {
        errorCallback_ = cb;
    }
private:
    EventLoop *loop_;
    const InetAddress peer_;
    const int sockfd_;
    bool connected_;
    bool started_;
    Channel channel_;
    NewConnectionCallback newConnectionCallback_;
    ErrorCallback errorCallback_;

    void handleWrite();
};

}
