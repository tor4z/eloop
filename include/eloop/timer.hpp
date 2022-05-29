#pragma once

#include <cassert>
#include "eloop/callback.hpp"
#include "eloop/channel.hpp"
#include "eloop/timestamp.hpp"
#include "eloop/noncopyable.hpp"

namespace eloop
{

class Timer: noncopyable
{
public:
    Timer(TimerCallback callback, Timestamp when, Nanosecond interval)
        : callback_(callback),
          when_(when),
          interval_(interval),
          repeat_(interval > Nanosecond::zero()),
          canceled_(false)
    {}

    void run()
    {
        if(callback_)
            callback_();
    }

    bool repeat() const
    {
        return repeat_;
    }

    bool expired(Timestamp now) const
    {
        return now >= when_;
    }

    Timestamp when() const
    {
        return when_;
    }

    void restart()
    {
        assert(repeat_);
        when_ += interval_;
    }
    
    void cancel()
    {
        assert(!canceled_);
        canceled_ = true;
    }

    bool canceled() const
    {
        return canceled_;
    }
private:
    TimerCallback callback_;
    Timestamp when_;
    const Nanosecond interval_;
    bool repeat_;
    bool canceled_;
};

}
