#pragma once
#include "Timestamp.h"
#include "CallBacks.h"
#include "Channel.h"
#include <set>
#include <vector>

class Timer;
class EventLoop;
class TimerId;

class TimerQueue
{
public:
    using Entry = std::pair<Timestamp, Timer *>;     // 时间戳和定时器指针
    using TimerList = std::set<Entry>;               // 定时器列表
    using ActiveTimer = std::pair<Timer *, int64_t>; // 定时器指针和序列号
    using ActiveTimerSet = std::set<ActiveTimer>;    // 活动定时器集合

    TimerQueue(EventLoop *loop);
    ~TimerQueue();
    TimerId addTimer(const TimerCallback &cb, Timestamp when, double interval);
    void cancel(TimerId timerId);

private:
    void handleRead();                                            //  处理定时器事件
    std::vector<Entry> getExpired(Timestamp now);                 // 获取所有过期定时器
    void reset(const std::vector<Entry> &expired, Timestamp now); // 重置定时器
    bool insert(Timer *timer);                                    // 插入定时器
    void resetTimerfd(int timerfd, Timestamp expiration);         // 重置定时器
    void addTimerInLoop(Timer *timer);                            // 在EventLoop中添加定时器
    void cancelInLoop(TimerId timerId);                           // 在EventLoop中取消定时器

private:
    EventLoop *loop_;                // 所属EventLoop
    const int timerfd_;              // 定时器文件描述符
    Channel timerfdChannel_;         // 定时器通道
    TimerList timers_;               // 定时器列表
    ActiveTimerSet activeTimers_;    // 活动定时器集合
    bool callingExpiredTimers_;      // 是否正在调用过期定时器
    ActiveTimerSet cancelingTimers_; // 取消定时器集合
};
