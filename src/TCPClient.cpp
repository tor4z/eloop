#include "eloop/TCPClient.hpp"
#include "eloop/callback.hpp"
#include "eloop/channel.hpp"
#include "eloop/connector.hpp"
#include "eloop/inetAddress.hpp"
#include "eloop/TCPConnection.hpp"
#include "eloop/eventLoop.hpp"
#include "eloop/log.hpp"
#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>


namespace eloop
{

TCPClient::TCPClient(EventLoop *loop, const InetAddress &peer)
    : loop_(loop),
      connected_(false),
      peer_(peer),
      retryTimer_(nullptr),
      connector_(new Connector(loop, peer)),
      connectionCallback_(defaultConnectionCallback),
      messageCallback_(defaultMessageCallback)
{
    connector_->setNewConnectionCallback(
        std::bind(
            &TCPClient::newConnection,
            this,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3
        )
    );
}


TCPClient::~TCPClient()
{
    if (connection_ && !connection_->disconnected())
        connection_->forceClose();
    if (retryTimer_ != nullptr)
        loop_->cancelTimer(retryTimer_);
}


void TCPClient::start()
{
    loop_->assertInLoopThread();
    connector_->start();
    retryTimer_ = loop_->runEvery(
        3s,
        [this]()
        {
            retry();
        }
    );
}


void TCPClient::retry()
{
    loop_->assertInLoopThread();
    if (connected_)
        return;
    
    WARN("TCPClient::retry reconnect %s ...", peer_.toIPPort().c_str());
    connector_ = std::make_unique<Connector>(loop_, peer_);
    connector_->setNewConnectionCallback(
        std::bind(
            &TCPClient::newConnection,
            this,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3
        )
    );
    connector_->start();
}


void TCPClient::newConnection(
    int connfd, const InetAddress &local, const InetAddress &peer
)
{
    loop_->assertInLoopThread();
    loop_->cancelTimer(retryTimer_);
    retryTimer_ = nullptr;
    connected_ = true;
    auto conn = std::make_shared<TCPConnection>(
        loop_, connfd, local, peer
    );
    connection_ = conn;
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
        std::bind(
            &TCPClient::closeConnection,
            this, std::placeholders::_1
        )
    );
    conn->connectEstablished();
    connectionCallback_(conn);
}


void TCPClient::closeConnection(const TCPConnectionPtr &conn)
{
    loop_->assertInLoopThread();
    assert(connection_ != nullptr);
    connection_.reset();
    connectionCallback_(conn);
}

}
