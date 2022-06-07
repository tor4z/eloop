#include <cassert>
#include <mutex>
#include <sys/eventfd.h>
#include <sys/types.h>
#include <unistd.h>
#include <syscall.h>
#include <signal.h>
#include <numeric>
#include <vector>

#include "eloop/log.hpp"
#include "eloop/channel.hpp"
#include "eloop/eventLoop.hpp"

namespace
{

__thread eloop::EventLoop *t_EventLoop = nullptr;

pid_t get_tid()
{
    return static_cast<pid_t>(::syscall(SYS_gettid));
}

class IgnoreSigPipe
{
public:
    IgnoreSigPipe()
    {
        ::signal(SIGPIPE, SIG_IGN);
    }
};

IgnoreSigPipe ignore;

}

namespace eloop
{

EventLoop::EventLoop()
    : tid_(get_tid()),
      quit_(false),
      doingPendingTasks_(false),
      poller_(this),
      wakeupFd_(::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)),
      wakeupChannel_(this, wakeupFd_),
      timerQueue_(this)
{
    if(wakeupFd_ == -1)
        SYSFATAL("EventLoop::eventfd()");
    
    wakeupChannel_.setReadCallback(
        [this]()
        {
            handleRead();
        }
    );
    wakeupChannel_.enableRead();

    assert(t_EventLoop == nullptr);
    t_EventLoop = this;
}

EventLoop::~EventLoop()
{
    assert(t_EventLoop == this);
    t_EventLoop = nullptr;
}

void EventLoop::loop()
{
    assertInLoopThread();
    TRACE("EventLoop %p polling", this);
    quit_ = false;

    while (!quit_)
    {
        activeChannels_.clear();
        poller_.poll(activeChannels_);
        for (auto channel: activeChannels_)
            channel->handleEvents();
        doPendingTasks();
    }

    TRACE("EventLoop %p quit.", this);
}

void EventLoop::quit()
{
    assert(!quit_);
    quit_ = true;
    if(!isInLoopThread())
        wakeup();
}

void EventLoop::runInLoop(const Task &task)
{
    if(isInLoopThread())
        task();
    else
        queueInLoop(task);
}

void EventLoop::runInLoop(Task &&task)
{
    if(isInLoopThread())
        task();
    else
        queueInLoop(std::move(task));
}

void EventLoop::queueInLoop(const Task &task)
{
    {
        std::lock_guard<std::mutex> guard(mutex_);
        pendingTasks_.push_back(task);
    }
    if(!isInLoopThread() || doingPendingTasks_)
        wakeup();
}


void EventLoop::queueInLoop(Task &&task)
{
    {
        std::lock_guard<std::mutex> gard(mutex_);
        pendingTasks_.push_back(std::move(task));
    }
    if(!isInLoopThread() || doingPendingTasks_)
        wakeup();
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof(one));
    if(n != sizeof(one))
        SYSERR(
            "EventLoop::wakeup() should write %lu bytes",
            sizeof(one)
        );
}

bool EventLoop::isInLoopThread()
{
    // tid_ is const
    return tid_ == get_tid();
}

Timer *EventLoop::runAt(Timestamp when, TimerCallback callback)
{
    return timerQueue_.addTimer(
        std::move(callback),
        when,
        Millisecond::zero()
    );
}

Timer *EventLoop::runAfter(Nanosecond interval, TimerCallback callback)
{
    return runAt(
        clock::now() + interval,
        std::move(callback)
    );
}

Timer *EventLoop::runEvery(Nanosecond interval, TimerCallback callback)
{
    return timerQueue_.addTimer(
        std::move(callback),
        clock::now() + interval,
        interval
    );
}

void EventLoop::cancelTimer(Timer *timer)
{
    timerQueue_.cancelTimer(timer);
}

void EventLoop::updateChannel(Channel *channel)
{
    assertInLoopThread();
    poller_.updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    assertInLoopThread();
    channel->disableAll();
}

void EventLoop::assertInLoopThread()
{
    assert(isInLoopThread());
}

void EventLoop::assertNotInLoopThread()
{
    assert(!isInLoopThread());
}


void EventLoop::doPendingTasks()
{
    assertInLoopThread();
    std::vector<Task> tasks;

    {
        std::lock_guard<std::mutex> guard(mutex_);
        tasks.swap(pendingTasks_);
    }

    doingPendingTasks_ = true;
    for (auto &task: tasks)
        task();
    doingPendingTasks_ = false;
}

void EventLoop::handleRead()
{
    uint64_t one;
    ssize_t n = ::read(wakeupFd_, &one, sizeof(one));
    if(n != sizeof(one))
        SYSERR(
            "EventLoop::handleRead() should ::read %lu bytes.",
            sizeof(one)
        );
}

}
