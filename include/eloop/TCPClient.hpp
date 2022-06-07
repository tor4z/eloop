#pragma once

#include "eloop/callback.hpp"
#include "eloop/channel.hpp"
#include "eloop/connector.hpp"
#include "eloop/inetAddress.hpp"
#include "eloop/noncopyable.hpp"
#include "eloop/timer.hpp"
#include <memory>


namespace eloop
{

class TCPClient: noncopyable
{
public:
    TCPClient(EventLoop *loop, const InetAddress &peer);
    ~TCPClient();
    void start();

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

    void setErrorCallback(const ErrorCallback &cb)
    {
        connector_->setErrorCallback(cb);
    }

private:
    using ConnectorPtr = std::unique_ptr<Connector>;

    EventLoop *loop_;
    bool connected_;
    const InetAddress peer_;
    Timer *retryTimer_;
    ConnectorPtr connector_;
    TCPConnectionPtr connection_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;

    void retry();
    void newConnection(
        int connfd, const InetAddress &locak, const InetAddress &peer
    );
    void closeConnection(const TCPConnectionPtr &conn);
};

}


