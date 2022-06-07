#include "eloop/TCPServer.hpp"
#include "eloop/TCPServerSingle.hpp"
#include "eloop/channel.hpp"
#include "eloop/inetAddress.hpp"
#include "eloop/callback.hpp"
#include "eloop/log.hpp"
#include "eloop/eventLoop.hpp"
#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>


namespace eloop
{

TCPServer::TCPServer(EventLoop *loop, const InetAddress &local)
    : baseLoop_(loop),
      numThread_(1),
      started_(false),
      local_(local),
      threadInitCallback_(defaultThreadInitCallback),
      connectionCallback_(defaultConnectionCallback),
      messageCallback_(defaultMessageCallback)
{
    INFO("Create TCPServer %s", local_.toIPPort().c_str());
}


TCPServer::~TCPServer()
{
    for (auto &loop: eventLoops_)
        loop->quit();
    for (auto &thread: threads_)
        thread->join();
    TRACE("TCPServer exit.");
}


void TCPServer::setNumThread(size_t n)
{
    baseLoop_->assertInLoopThread();
    assert(n > 0);
    assert(!started_);
    numThread_ = n;
    eventLoops_.resize(n);
}


void TCPServer::start()
{
    if (started_.exchange(true))
        return;
    baseLoop_->runInLoop(
        [=]()
        {
            startInLoop();
        }
    );
}


void TCPServer::startInLoop()
{
    INFO(
        "TCPServer::startInLoop %s with %lu eventLoop thread(s)",
        local_.toIPPort().c_str(), numThread_
    );

    baseServer_ = std::make_unique<TCPServerSingle>(baseLoop_, local_);
    baseServer_->setConnectionCallback(connectionCallback_);
    baseServer_->setMessageCallback(messageCallback_);
    baseServer_->setWriteCompleteCallback(writeCompleteCallback_);
    threadInitCallback_(0);
    baseServer_->start();

    for (size_t i = 1; i< numThread_; ++i)
    {
        auto thread = new std::thread(
            std::bind(
                &TCPServer::runInLoop,
                this, i
            )
        );
        {
            std::unique_lock<std::mutex> lck(mutex_);
            while (eventLoops_[i] == nullptr)
                cond_.wait(lck);
        }
        threads_.emplace_back(thread);
    }
}


void TCPServer::runInLoop(size_t index)
{
    EventLoop loop;
    TCPServerSingle server(&loop, local_);

    server.setConnectionCallback(connectionCallback_);
    server.setMessageCallback(messageCallback_);
    server.setWriteCompleteCallback(writeCompleteCallback_);

    {
        std::lock_guard<std::mutex> guard(mutex_);
        eventLoops_[index] = &loop;
        cond_.notify_one();
    }

    threadInitCallback_(index);
    server.start();
    loop.loop();
    eventLoops_[index] = nullptr;
}

}
