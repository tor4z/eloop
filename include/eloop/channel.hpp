#pragma once

#include <functional>
#include <memory>
#include <sys/epoll.h>
#include "eloop/noncopyable.hpp"


namespace eloop
{

class EventLoop;

class Channel: noncopyable
{
public:
    using ReadCallback = std::function<void()>;
    using WriteCallback = std::function<void()>;
    using CloseCallback = std::function<void()>;
    using ErrorCallback = std::function<void()>;

    bool polling;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    void setReadCallback(const ReadCallback &cb)
    {
        readCallback_ = cb;
    }

    void setWriteCallback(const WriteCallback &cb)
    {
        writeCallback_ = cb;
    }

    void setCloseCallback(const CloseCallback &cb)
    {
        closeCallback_ = cb;
    }

    void setErrorCallback(const ErrorCallback &cb)
    {
        errorCallback_ = cb;
    }

    int fd() const 
    {
        return fd_;
    }

    bool isNoneEvents() const
    {
        return events_ == 0;
    }
    
    unsigned events() const
    {
        return events_;
    }

    void setRevents(unsigned revents)
    {
        revents_ = revents;
    }

    void enableRead()
    {
        events_ |= (EPOLLIN | EPOLLPRI);
        update();
    }

    void enableWrite()
    {
        events_ |= EPOLLOUT;
        update();
    }

    void dsiableRead()
    {
        events_ &= ~EPOLLIN;
        update();
    }

    void disableWrite()
    {
        events_ &= ~EPOLLOUT;
        update();
    }

    void disableAll()
    {
        events_ = 0;
        update();
    }

    bool isReading()
    {
        return events_ & EPOLLIN;
    }

    bool isWriting()
    {
        return events_ & EPOLLOUT;
    }

    void handleEvent();
    void tie(const std::shared_ptr<void>& obj);
private:
    void update();
    void remove();

    EventLoop *loop_;
    int fd_;

    std::weak_ptr<void> tie_;
    bool tied_;

    unsigned events_;
    unsigned revents_;

    bool hlandlingEvents_;

    ReadCallback readCallback_;
    WriteCallback writeCallback_;
    CloseCallback closeCallback_;
    ErrorCallback errorCallback_;
};

}

