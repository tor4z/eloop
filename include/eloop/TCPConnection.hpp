#pragma once

#include <any>
#include <string_view>
#include "eloop/noncopyable.hpp"
#include "eloop/buffer.hpp"
#include "eloop/callback.hpp"
#include "eloop/channel.hpp"
#include "eloop/inetAddress.hpp"


namespace eloop
{
class EventLoop;

class TCPConnection: noncopyable,
                     std::enable_shared_from_this<TCPConnection>
{
public:
    TCPConnection(
        EventLoop *loop,
        int sockfd,
        const InetAddress &local,
        const InetAddress &peer
    );
    ~TCPConnection();

    void setMessageCallback(const MessageCallback &cb)
    {
        messageCallback_ = cb;
    }

    void setWriteCompleteCallback(const WriteCompleteCallback &cb)
    {
        writeCompleteCallback_ = cb;
    }

    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t mark)
    {
        highWaterMarkCallback_ = cb;
        highWaterMark_ = mark;
    }

    void setCloseCallback(const CloseCallback &cb)
    {
        closeCallback_ = cb;
    }

    const InetAddress &local() const
    {
        return local_;
    }

    const InetAddress &peer() const
    {
        return peer_;
    }

    std::string name() const
    {
        return peer_.toIPPort() + "->" + local_.toIPPort();
    }

    void setContext(std::any &context)
    {
        context_ = context;
    }

    const std::any &getContext() const
    {
        return context_;
    }

    std::any &getContext()
    {
        return context_;
    }

    bool isReading()
    {
        return channel_.isReading();
    }

    const Buffer &inputBuffer() const
    {
        return inputBuffer_;
    }

    const Buffer &outputBuffer() const
    {
        return outputBuffer_;
    }

    // I/O operations are thread safe
    void send(std::string_view data);
    void send(const char* data, size_t len);
    void send(Buffer &buff);
    void shutdown();
    void forceClose();

    void stopRead();
    void startRead();

    void connectEstablished();
    bool connected() const;
    bool disconnected() const;
private:
    EventLoop *loop_;
    const int sockfd_;
    Channel channel_;
    int state_;
    InetAddress local_;
    InetAddress peer_;
    Buffer inputBuffer_;
    Buffer outputBuffer_;
    size_t highWaterMark_;
    std::any context_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;

    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const char *data, size_t len);
    void sendInLoop(const std::string &message);
    void shutdownInLoop();
    void forceCloseInLoop();

    int stateAtomicGetAndSet(int newState);
};

}
