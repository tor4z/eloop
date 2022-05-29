#pragma once

#include <memory>
#include <set>
#include <vector>
#include "eloop/timer.hpp"
#include "eloop/channel.hpp"
#include "eloop/noncopyable.hpp"

namespace eloop
{

class TimerQueue: noncopyable
{
public:
    explicit TimerQueue(EventLoop *loop);
    ~TimerQueue();

    Timer *addTimer(TimerCallback cb, Timestamp when, Nanosecond interval);
    void cancelTimer(Timer *timer);
private:
    using Entry = std::pair<Timestamp, Timer>;
    using TimerList = std::set<Entry>;

    EventLoop *loop_;
    const int timerfd_;
    Channel timerChannel_;
    TimerList timers_;

    void handleRead();
    std::vector<Entry> getExpired(Timestamp now);
};

}
