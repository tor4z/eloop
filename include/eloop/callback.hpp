#pragma once

#include <memory>
#include <functional>

namespace eloop
{

class Buffer;
class TCPConnection;
class InetAddress;

using TCPConnectionPtr = std::shared_ptr<TCPConnection>;
using CloseCallback = std::function<void(const TCPConnectionPtr&)>;
using ConnectionCallback = std::function<void(const TCPConnectionPtr&)>;
using WriteCompleteCallback = std::function<void(const TCPConnectionPtr&)>;
using HighWaterMarkCallback = std::function<void(const TCPConnectionPtr, size_t)>;
using MessageCallback = std::function<void(const TCPConnectionPtr&, Buffer&)>;

using ErrorCallback = std::function<void()>;
using NewConnectionCallback = std::function<
        void(
            int sockfd,
            const InetAddress &local,
            const InetAddress &peer
        )
    >;
using Task = std::function<void()>;
using ThreadInitCallback = std::function<void(size_t index)>;
using TimerCallback = std::function<void()>;

void defaultThreadInitCallback(size_t index);
void defaultConnectionCallback(const TCPConnectionPtr &conn);
void defaultMessageCallback(const TCPConnectionPtr &conn, Buffer &buffer);

}
