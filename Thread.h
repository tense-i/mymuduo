#pragma once
#include "noncopyable.h"
#include "functional"
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <unistd.h>

// Thread在项目中的作用？
// a:Thread类是一个线程类，用于创建线程对象，管理线程的生命周期。
// q:为什么要继承noncopyable类？
// a:因为Thread类的对象不可拷贝，所以继承noncopyable类，禁止拷贝构造函数和赋值构造函数。

class Thread : noncopyable
{
public:
    using ThreadFunc = std::function<void()>; // 线程任务函数
    explicit Thread(ThreadFunc, const std::string &name = std::string());
    ~Thread();

    void start();
    void join();

    bool started() const { return started_; }
    pid_t tid() const { return tid_; }
    const std::string &name() const { return name_; }

    // 返回创建的线程数
    // q 为什么要用static修饰？
    // a:因为numCreated_是一个全局变量，所有线程共享
    static int numCreated() { return numCreated_; }

private:
    bool started_;
    bool joined_;
    // 使用SharedPtr管理线程对象
    // q:为什么要用SharedPtr管理线程对象？
    // a:因为如果使用 std::thread thread_之间定义对象而不是对象指针，thread_线程在Thread对象创建后立即启动，而不是在start()函数中启动。我们要手动管理线程的生命周期，所以使用SharedPtr管理线程对象。
    std::shared_ptr<std::thread> thread_;
    pid_t tid_;
    ThreadFunc func_;                   // 线程函数
    std::string name_;                  // 线程名
    static std::atomic_int numCreated_; // 原子类型、创建的线程数

    void setDefaultName();
};