#include <sys/timerfd.h>
#include <strings.h>
#include <unistd.h>
#include <ratio>

#include "eloop/log.hpp"
#include "eloop/eventLoop.hpp"
#include "eloop/timerQueue.hpp"


namespace
{

int timerfdCreate()
{
    int fd = timerfd_create(
        CLOCK_MONOTONIC,
        TFD_NONBLOCK | TFD_CLOEXEC
    );
    if(fd == -1)
        eloop::SYSFATAL("timer_create()");
    return fd;
}

void timerfdRead(int fd)
{
    uint64_t val;
    ssize_t n = read(fd, &val, sizeof(val));
    if(n != sizeof(val))
        eloop::ERROR("TimerfdRead get %ld, not %lu", n, sizeof(val));
}

struct timespec durationFromNow(eloop::Timestamp when)
{
    using namespace eloop;
    struct timespec ret;
    eloop::Nanosecond ns = when - clock::now();
    if(ns < 1ms)
        ns = 1ms;
    
    ret.tv_sec = static_cast<time_t>(ns.count() / std::nano::den);
    ret.tv_nsec = ns.count() % std::nano::den;
    return ret;
}

void timerfdSet(int fd, eloop::Timestamp when)
{
    struct itimerspec oldTime, newTime;
    bzero(&oldTime, sizeof(oldTime));
    bzero(&newTime, sizeof(newTime));
    newTime.it_value = durationFromNow(when);

    int ret = timerfd_settime(fd, 0, &newTime, &oldTime);
    if(ret == -1)
        eloop::SYSERR("timerfd_settime()");
}

}

namespace eloop
{

TimerQueue::TimerQueue(EventLoop *loop)
    : loop_(loop),
      timerfd_(timerfdCreate()),
      timerChannel_(loop, timerfd_)
{

}

}
