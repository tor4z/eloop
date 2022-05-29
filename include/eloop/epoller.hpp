#pragma once

#include <vector>
#include <sys/epoll.h>
#include "eloop/noncopyable.hpp"

namespace eloop
{

class Channel;
class EventLoop;

class EPoller: noncopyable
{
public:
    using ChannelList = std::vector<Channel>;

    explicit EPoller(EventLoop *loop);
    ~EPoller();

    void poll(ChannelList &activeChannels);
    void updateChannel(Channel &channel);
private:
    void updateChannel(int op, Channel &channel);
    EventLoop* loop_;
    std::vector<struct epoll_event> events_;
    int epollfd_;
};

}
