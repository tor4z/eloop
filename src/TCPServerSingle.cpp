#include <functional>
#include "eloop/log.hpp"
#include "eloop/buffer.hpp"
#include "eloop/eventLoop.hpp"
#include "eloop/TCPConnection.hpp"
#include "eloop/TCPServerSingle.hpp"


namespace eloop
{

TCPServerSingle::TCPServerSingle(
    EventLoop *loop,
    const InetAddress &local
): loop_(loop),
   acceptor_(loop, local)
{
    acceptor_.setNewConnectionCallback(
        std::bind(
            &TCPServerSingle::newConnection,
            this,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3
        )
    );
}

void TCPServerSingle::start()
{
    acceptor_.listen();
}

void TCPServerSingle::newConnection(
    int connfd,
    const InetAddress &local,
    const InetAddress &peer
)
{
    loop_->assertInLoopThread();
    auto conn = std::make_shared<TCPConnection>(
        loop_, connfd, local, peer
    );
    connections_.insert(conn);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
        std::bind(
            &TCPServerSingle::closeConnection,
            this, std::placeholders::_1
        )
    );
    conn->connectEstablished();
    connectionCallback_(conn);
}

void TCPServerSingle::closeConnection(const TCPConnectionPtr &conn)
{
    loop_->assertInLoopThread();
    size_t ret = connections_.erase(conn);
    assert(ret == 1);
    (void)ret;
    connectionCallback_(conn);
}

}
