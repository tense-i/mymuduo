
#pragma once

#include "noncopyable.h"
#include "Thread.h"

#include <functional>
#include <mutex>
#include <condition_variable>
#include <string>

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

    EventLoop *loop_; // EventLoopThread对应的EventLoop对象，即一个线程对应一个EventLoop对象
    bool exiting_;
    Thread thread_; // EventLoopThread对象内部持有一个Thread对象，用于创建一个新的线程

    /**
     * @brief 用于线程同步，用于等待线程创建完毕（等待EventLoop对象创建完毕）
     */
    std::mutex mutex_;
    std::condition_variable cond_;

    // 可选的回调函数，用于在EventLoop对象创建完毕后执行一些初始化操作
    ThreadInitCallback callback_;
};