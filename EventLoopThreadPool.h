#pragma once
#include "noncopyable.h"

#include <functional>
#include <string>
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread;

// q: EventLoopThreadPool类的作用是什么？
// a: EventLoopThreadPool类的作用是创建指定数量的EventLoop对象，用于多线程中的事件循环处理。
// a: 一个程序中可以有多个EvenetLoopPool对象，每个EventLoopPool对象中可以有多个EventLoop对象。
// a: 同一管理多个EventLoop对象，用于多线程中的事件循环处理。
class EventLoopThreadPool : noncopyable
{

public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    EventLoopThreadPool(EventLoop *baseLoop, const std::string &name);
    ~EventLoopThreadPool();

    void start(const ThreadInitCallback &cb);
    EventLoop *getNextLoop();
    std::vector<EventLoop *> getAllLoops();

    bool started() const { return started_; }
    const std::string name() const { return name_; }

    void setThreadNum(int numThreads) { numThreads_ = numThreads; }

private:
    // 服务端的主EventLoop、单线程时的EventLoop
    EventLoop *baseLoop_;
    std::string name_;
    bool started_;
    int numThreads_; // 线程池中线程的数量
    // 线程池的下一个要执行的EventLoop-0开始
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_; // 该EventLoopThreadPool线程池中的线程集合
    std::vector<EventLoop *> loops_;                        // 该EventLoopThreadPool线程池中的EventLoop集合
};
