#include <chrono>
#include "eloop/log.hpp"
#include "eloop/eventLoop.hpp"

using namespace eloop;

int main()
{
    eloop::EventLoop loop;
    using namespace std::literals::chrono_literals;

    loop.runEvery(1s, 
        []()
        {
            INFO("run every 1s");
        }
    );

    loop.loop();
    return 0;
}
