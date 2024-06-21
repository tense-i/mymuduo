
#pragma once

#include "noncopyable.h"
#include "Thread.h"
#include <mutex>
#include <condition_variable>

class EventLoop;

// q EventLoopThread类的作用是什么？
// a EventLoopThread类的作用是创建一个新的线程，线程中运行一个新的EventLoop对象，实现一个线程对应一个EventLoop对象。
class EventLoopThread : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(), const std::string &name = std::string());

    ~EventLoopThread();
    EventLoop *startLoop();

private:
    void threadFunc();

    EventLoop *loop_;
    bool exiting_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;
};