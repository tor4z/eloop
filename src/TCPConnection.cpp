#include <cassert>
#include <unistd.h>
#include "eloop/log.hpp"
#include "eloop/eventLoop.hpp"
#include "eloop/TCPConnection.hpp"


namespace eloop
{

enum ConnectionState
{
    kConnecting,
    kConnected,
    kDisconnecting,
    kDisconnected
};

void defaultThreadInitCallback(size_t index)
{
    TRACE("EventLoop thread #%lu startted", index);
}

void defaultConnectionCallback(const TCPConnectionPtr &conn)
{
    INFO(
        "connection %s -> %s %s",
        conn->peer().toIPPort().c_str(),
        conn->local().toIPPort().c_str(),
        conn->connected()? "up": "down"
    );
}

void defaultMessageCallback(
    const TCPConnectionPtr &conn,
    Buffer &buffer
)
{
    TRACE(
        "connection %s -> %s recv %lu bytes",
        conn->peer().toIPPort().c_str(),
        conn->local().toIPPort().c_str(),
        buffer.readableBytes()
    );
    buffer.retrieveAll();
}


TCPConnection::TCPConnection(
    EventLoop *loop,
    int sockfd,
    const InetAddress &local,
    const InetAddress &peer
): loop_(loop),
   sockfd_(sockfd),
   channel_(loop_, sockfd_),
   state_(kConnecting),
   local_(local),
   peer_(peer),
   highWaterMark_(0)
{
    channel_.setReadCallback([this](){handleRead();});
    channel_.setWriteCallback([this](){handleWrite();});
    channel_.setCloseCallback([this](){handleClose();});
    channel_.setErrorCallback([this](){handleError();});

    TRACE(
        "TCPConnection() %s fd=%d",
        name().c_str(),
        sockfd_
    );
}

TCPConnection::~TCPConnection()
{
    assert(state_ == kDisconnected);
    ::close(sockfd_);

    TRACE(
        "~TCPConnection(), %s, fd=%d",
        name().c_str(),
        sockfd_
    );
}

void TCPConnection::connectEstablished()
{
    assert(state_ = kConnecting);
    state_ = kConnected;
    channel_.tie(shared_from_this());
    channel_.enableRead();
}

bool TCPConnection::connected() const
{
    return state_ == kConnected;
}

bool TCPConnection::disconnected() const
{
    return state_ == kDisconnected;
}

void TCPConnection::send(std::string_view data)
{
    send(data.begin(), data.length());
}

void TCPConnection::send(const char *data, size_t len)
{
    if(state_ != kConnected)
    {
        WARN(
            "TCPConnection::send() not connected, give up send."
        );
        return;
    }
    
    if(loop_->isInLoopThread())
    {
        sendInLoop(data, len);
    }
    else
    {
        loop_->queueInLoop(
            [ptr = shared_from_this(), str = std::string(data, data + len)]()
            {
                ptr->sendInLoop(str);
            }
        );
    }
}

void TCPConnection::send(Buffer &buffer)
{
    if(state_ != kConnected)
    {
        WARN(
            "TCPConnection::send() not connected, give up send."
        );
        return;
    }
     if(loop_->isInLoopThread())
     {
         sendInLoop(buffer.peek(), buffer.readableBytes());
         buffer.retrieveAll();
     }
     else
     {
         loop_->queueInLoop(
             [ptr = shared_from_this(), str = buffer.retrieveAllAsString()]()
             {
                 ptr->sendInLoop(str);
             }
         );
     }
}

void TCPConnection::sendInLoop(const char *data, size_t len)
{
    loop_->assertInLoopThread();
    
    if(state_ == kConnected)
    {
        WARN(
            "TCPConnection::sendInLoop() dsiconncted, give up send."
        );
        return;
    }

    ssize_t n = 0;
    size_t remain = len;
    bool faultError = false;

    if(!channel_.isWriting())
    {
        assert(outputBuffer_.readableBytes() == 0);
        n = ::write(sockfd_, data, len);
        if(n == -1)
        {
            if(errno != EAGAIN)
            {
                SYSERR("TCPConnection::write().");
                if(errno == EPIPE || errno == ECONNRESET)
                    faultError = true;
            }
            n = 0;
        }
        else
        {
            remain -= static_cast<size_t>(n);
            if(remain == 0 && writeCompleteCallback_)
            {
                loop_->queueInLoop(
                    std::bind(
                        writeCompleteCallback_,
                        shared_from_this()
                    )
                );
            }
        }
    }

    // TODO: add highWaterMark
    if (!faultError && remain > 0)
    {
        if(highWaterMark_)
        {
            size_t oldLen = outputBuffer_.readableBytes();
            size_t newLen = oldLen + remain;
            if (oldLen < highWaterMark_ && newLen >= highWaterMark_)
                loop_->queueInLoop(
                    std::bind(
                        highWaterMarkCallback_,
                        shared_from_this(), newLen
                    )
                );
        }
        outputBuffer_.append(data + n, remain);
        channel_.enableRead();
    }
}

void TCPConnection::sendInLoop(const std::string &message)
{
    sendInLoop(message.data(), message.size());
}

void TCPConnection::shutdownInLoop()
{
    loop_->assertInLoopThread();
    if(state_ != kDisconnected && !channel_.isWriting())
    {
        if(::shutdown(sockfd_, SHUT_WR) == -1)
            SYSERR("TCPConnection::shutdown()");
    }
}

void TCPConnection::forceCloseInLoop()
{
    loop_->assertInLoopThread();
    if(state_ != kDisconnected)
        handleClose();
}

void TCPConnection::shutdown()
{
    assert(state_ <= kDisconnected);

    if(stateAtomicGetAndSet(kDisconnecting) == kConnected)
    {
        if(loop_->isInLoopThread())
            shutdownInLoop();
        else
            loop_->queueInLoop(
                std::bind(
                    &TCPConnection::shutdownInLoop,
                    shared_from_this()
                )
            );
    }
}

void TCPConnection::forceClose()
{
    if(state_ != kDisconnected)
    {
        if(stateAtomicGetAndSet(kDisconnecting) == kConnected)
        {
            loop_->queueInLoop(
                std::bind(
                    &TCPConnection::forceCloseInLoop,
                    shared_from_this()
                )
            );
        }
    }
}

int TCPConnection::stateAtomicGetAndSet(int newState)
{
    return __atomic_exchange_n(&state_, newState, __ATOMIC_SEQ_CST);
}

void TCPConnection::stopRead()
{
    loop_->runInLoop(
        [this]()
        {
            if(channel_.isReading())
                channel_.dsiableRead();
        }
    );
}

void TCPConnection::startRead()
{
    loop_->runInLoop(
        std::bind(
            [this]()
            {
                if(!channel_.isReading())
                    channel_.enableRead();
            }
        )
    );
}

void TCPConnection::handleRead()
{
    loop_->assertInLoopThread();
    assert(state_ != kDisconnected);
    int savedErrno;
    ssize_t n = inputBuffer_.readFD(sockfd_, &savedErrno);
    if(n == -1)
    {
        errno = savedErrno;
        SYSERR("TCPConnection::read()");
        handleError();
    }
    else if(n == 0)
    {
        handleClose();
    }
    else
    {
        messageCallback_(shared_from_this(), inputBuffer_);
    }
}

void TCPConnection::handleWrite()
{
    if(state_ == kDisconnected)
    {
        WARN(
            "TCPConnection::handleWrite() dsiconnected,"
            "give up write %lu bytes.",
            outputBuffer_.readableBytes()
        );
        return;
    }

    assert(outputBuffer_.readableBytes() > 0);
    assert(channel_.isWriting());
    size_t n = ::write(
        sockfd_, outputBuffer_.peek(), outputBuffer_.readableBytes()
    );

    if(n == -1)
    {
        SYSERR("TCPConnection::write()");
    }
    else
    {
        outputBuffer_.retrieve(static_cast<size_t>(n));
        if(outputBuffer_.readableBytes() == 0)
        {
            channel_.disableWrite();
            if(state_ == kDisconnecting)
                shutdownInLoop();
            if(writeCompleteCallback_)
                loop_->queueInLoop(
                    std::bind(
                        writeCompleteCallback_,
                        shared_from_this()
                    )
                );
        }
    }
}

void TCPConnection::handleClose()
{
    loop_->assertInLoopThread();
    assert(state_ == kConnected || state_ == kDisconnecting);
    state_ = kDisconnected;
    loop_->removeChannel(&channel_);
    closeCallback_(shared_from_this());
}

void TCPConnection::handleError()
{
    int err;
    socklen_t len = sizeof(err);
    int ret = getsockopt(sockfd_, SOL_SOCKET, SO_ERROR, &err, &len);
    if(ret != -1)
        errno = err;
    SYSERR("TCPConnection::handleError()");
}

}
