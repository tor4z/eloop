#pragma once

#include <chrono>


namespace eloop
{
using namespace std::literals::chrono_literals;

using Nanosecond = std::chrono::nanoseconds;
using Microsecond = std::chrono::microseconds;
using Millisecond = std::chrono::milliseconds;
using Second = std::chrono::seconds;
using Minute = std::chrono::minutes;
using Hour = std::chrono::hours;
using Timestamp = std::chrono::time_point<std::chrono::system_clock, Nanosecond>;


namespace clock
{

inline Timestamp now()
{
    return std::chrono::system_clock::now();
}

inline Timestamp nowAfter(Nanosecond interval)
{
    return now() + interval;
}

inline Timestamp nowBefore(Nanosecond interval)
{
    return now() - interval;
}

}


template<typename T>
struct IntervalTypeCheckImpl
{
    static constexpr bool value =
        std::is_same<T, Nanosecond>::value ||
        std::is_same<T, Microsecond>::value ||
        std::is_same<T, Millisecond>::value ||
        std::is_same<T, Second>::value ||
        std::is_same<T, Minute>::value ||
        std::is_same<T, Hour>::value;
};

#define IntervalTypeCheck(T) \
    static_assert(IntervalTypeCheckImpl<T>::value, "Bad interval type.")

}
