#pragma once
#include "noncopyable.h"

#include <functional>
#include <string>
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
private:
    // 不使用并发时的根EventLoop
    EventLoop *baseLoop_;
    std::string name_;
    bool started_;
    int numThreads_;
    // 线程池的下一个要执行的EventLoop-0开始
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop *> loops_;

public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    EventLoopThreadPool(EventLoop *baseLoop, const std::string &name);
    ~EventLoopThreadPool();

    void start(const ThreadInitCallback &cb);
    EventLoop *getNextLoop();
    std::vector<EventLoop *> getAllLoops();

    bool started() const { return started_; }
    const std::string name() const { return name_; }
};
