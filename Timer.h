#pragma once
#include "noncopyable.h"
#include "CallBacks.h"
#include "Timestamp.h"

#include <atomic>
class Timer : noncopyable
{
private:
    const TimerCallback callback_;
    Timestamp expiration_;   // 过期时间
    const double interval_;  // 间隔时间
    const bool repeat_;      // 是否重复
    const int64_t sequence_; // 序列号
    // static std::atomic_int
    static std::atomic_int64_t s_numCreated_; // 已创建的定时器数量
public:
    Timer(TimerCallback cb, Timestamp when, double interval);
    ~Timer();

    void run() const { callback_(); }

    Timestamp expiration() const { return expiration_; }
    bool repeat() const { return repeat_; }
    int64_t sequence() const { return sequence_; }

    static int64_t numCreated() { return s_numCreated_; }

    void restart(Timestamp now); // 重启定时器
};
