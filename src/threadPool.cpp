#include <cassert>
#include "eloop/log.hpp"
#include "eloop/threadPool.hpp"


namespace eloop
{

ThreadPool::ThreadPool(
    size_t numThreads,
    size_t maxQueueSize,
    const ThreadInitCallback &cb
): maxQueueSize_(maxQueueSize),
   running_(true),
   threadInitCallback_(cb)
{
    assert(maxQueueSize_ > 0);
    for (size_t i = 0; i < numThreads; i++)
    {
        threads_.emplace_back(
            new std::thread(
                [this, i]()
                    {
                        runInThread(i);
                    }
            )
        );
    }
    TRACE(
        "ThreadPool(), numThreads %lu, maxQueueSize %lu",
        numThreads, maxQueueSize_
    );
}

ThreadPool::~ThreadPool()
{
    if(running_)
        stop();
    TRACE("~ThreadPool()");
}

void ThreadPool::runTask(const Task &task)
{
    assert(running_);
    
    if(threads_.empty())
    {
        task();
    }
    else
    {
        std::unique_lock<std::mutex> lck(mutex_);
        while (taskQueue_.size() >= maxQueueSize_)
            notFull_.wait(lck);
        taskQueue_.push_back(task);
        notEmpty_.notify_one();
    }
}

void ThreadPool::runTask(Task &&task)
{
    assert(running_);

    if(threads_.empty())
    {
        task();
    }
    else
    {
        std::unique_lock<std::mutex> lck(mutex_);
        while (taskQueue_.size() >= maxQueueSize_)
            notFull_.wait(lck);
        taskQueue_.push_back(task);
        notEmpty_.notify_one();
    }
}

void ThreadPool::stop()
{
    assert(running_);
    running_ = false;

    {
        std::lock_guard<std::mutex> guard(mutex_);
        notEmpty_.notify_all();
    }
    for (auto &thread: threads_)
        thread->join();
}

void ThreadPool::runInThread(size_t index)
{
    if(threadInitCallback_)
        threadInitCallback_(index);

    while (running_)
    {
        if(Task task = take())
            task();
    }   
}

Task ThreadPool::take()
{
    std::unique_lock<std::mutex> lck(mutex_);
    while (taskQueue_.empty() && running_)
        notEmpty_.wait(lck);
    
    Task task;
    if(!taskQueue_.empty())
    {
        task = taskQueue_.front();
        taskQueue_.pop_front();
        notFull_.notify_one();
    }
    return task;
}

}
