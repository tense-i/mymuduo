#include "TimerQueue.h"
#include "Logger.h"
#include "Timer.h"
#include "TimerId.h"

#include "EventLoop.h"
#include <cstring>
#include <sys/timerfd.h>
#include <unistd.h>
#include <fcntl.h> // Add this line to include the missing header file

/**
 * 创建定时器文件描述符

*/
int createTimerfd()
{
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0)
    {
        LOG_FATAL("Failed in timerfd_create");
    }
    return timerfd;
}

/**
 * 读取定时器文件描述符

*/
void readTimerfd(int timerfd, Timestamp now)
{
    uint64_t howmany;                                      // 定时器到期次数
    ssize_t n = ::read(timerfd, &howmany, sizeof howmany); // 读取定时器文件描述符
    LOG_DEBUG("TimerQueue::handleRead() %lu at %s\n", howmany, now.toFormattedString().c_str());
    if (n != sizeof howmany)
    {
        LOG_ERROR("TimerQueue::handleRead() reads %lu bytes instead of 8\n", n);
    }
}

/**
 * 计算超时时间
 */
struct timespec howMuchTimeFromNow(Timestamp when)
{
    // 计算目标时间和当前时间的差值，以微秒为单位
    int64_t microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();

    // 如果时间差小于 100 微秒，设为 100 微秒
    if (microseconds < 100)
    {
        microseconds = 100;
    }

    // 将时间差转换为 timespec 结构体
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);         // 秒
    ts.tv_nsec = static_cast<long>((microseconds % Timestamp::kMicroSecondsPerSecond) * 1000); // 纳秒

    return ts;
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
    std::vector<Entry> expired; // 过期定时器列表
    // reinterpret_cast<Timer *>(UINTPTR_MAX) 是为了在 timers_ 中找到第一个未到期的定时器
    // 关于reiterpret_cast的用法，可以参笔记
    Entry sentry(now, reinterpret_cast<Timer *>(UINTPTR_MAX)); // 超时时间和定时器指针

    // 返回第一个未到期的定时器的迭代器
    TimerList::iterator end = timers_.lower_bound(sentry);
    std::copy(timers_.begin(), end, back_inserter(expired)); // 将到期的定时器加入到 expired 中

    // 从 timers_ 中移除到期的定时器
    timers_.erase(timers_.begin(), end);

    for (const Entry &it : expired)
    {
        ActiveTimer timer(it.second, it.second->sequence());
        size_t n = activeTimers_.erase(timer);
    }
    return expired; // 返回到期的定时器
}

/**
 * 重置定时器
 * @param expired 过期定时器列表
 * @param now 读事件发生当前时间
 */
void TimerQueue::reset(const std::vector<Entry> &expired, Timestamp now)
{
    Timestamp nextExpire;

    for (const Entry &it : expired)
    {
        ActiveTimer timer(it.second, it.second->sequence()); // 用已过期的定时器构造活动定时器
        // 如果是重复定时器，并且不在取消定时器集合中
        if (it.second->repeat() && cancelingTimers_.find(timer) == cancelingTimers_.end())
        {
            it.second->restart(now); // 重启定时器
            insert(it.second);       // 插入定时器
        }
        else
        {
            // 释放定时器
            delete it.second;
        }
    }
    if (!timers_.empty())
    {
        nextExpire = timers_.begin()->first; // 获取最早到期的定时器
    }

    if (nextExpire.valid())
    {
        //
        resetTimerfd(timerfd_, nextExpire); // 重置定时器
    }
}

/**
 * 插入定时器
 * @return 是否最早到期的定时器改变
 */
bool TimerQueue::insert(Timer *timer)
{
    bool earliestChanged = false;         // 是否最早到期的定时器改变
    Timestamp when = timer->expiration(); // 获取定时器到期时间
    TimerList::iterator it = timers_.begin();
    // 如果 timers_ 为空或者 when 小于 timers_ 中的第一个定时器的到期时间
    if (it == timers_.end() || when < it->first)
    {
        earliestChanged = true; // 最早到期的定时器改变
    }
    {
        // 插入定时器
        std::pair<TimerList::iterator, bool> result = timers_.insert(Entry(when, timer));
    }
    {
        // 插入活动定时器
        std::pair<ActiveTimerSet::iterator, bool> result = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
    }
    return earliestChanged;
}

void TimerQueue::resetTimerfd(int timerfd, Timestamp expiration)
{
    struct itimerspec newValue;
    struct itimerspec oldValue;
    memset(&newValue, 0, sizeof newValue);                         // Zero-initialize the newValue structure
    memset(&oldValue, 0, sizeof oldValue);                         // Zero-initialize the oldValue structure
    newValue.it_value = howMuchTimeFromNow(expiration);            // 设置超时时间
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue); // 设置定时器超时时间
    if (ret)
    {
        LOG_ERROR("timerfd_settime()");
    }
}

void TimerQueue::addTimerInLoop(Timer *timer)
{
    if (loop_->isInLoopThread())
    {
        bool earliestChanged = insert(timer); // 插入定时器
        // 如果最早到期的定时器改变
        if (earliestChanged)
        {
            resetTimerfd(timerfd_, timer->expiration()); // 重置定时器
        }
    }
}

void TimerQueue::cancelInLoop(TimerId timerId)
{
    if (!loop_->isInLoopThread())
    {
        LOG_FATAL("cancelInLoop() is not in the loop thread!");
    }
    ActiveTimer timer(timerId.timer_, timerId.sequence_); // 构造活动定时器
    ActiveTimerSet::iterator it = activeTimers_.find(timer);
    // 如果定时器在活动定时器集合中
    if (it != activeTimers_.end())
    {
        size_t n = timers_.erase(Entry(it->first->expiration(), it->first)); // 从定时器列表中移除定时器
        delete it->first;                                                    // 释放定时器
        activeTimers_.erase(it);                                             // 从活动定时器集合中移除定时器
    }
    else if (callingExpiredTimers_) // 如果正在调用到期的定时器
    {
        // 将定时器加入到取消定时器集合中
        cancelingTimers_.insert(timer);
    }
}

void TimerQueue::handleRead()
{
    Timestamp now(Timestamp::now());
    readTimerfd(timerfd_, now);
    std::vector<Entry> expired = getExpired(now); // 获取到期的定时器
    callingExpiredTimers_ = true;                 // 正在调用到期的定时器
    cancelingTimers_.clear();                     // 清空取消定时器集合
    for (const Entry &it : expired)
    {
        it.second->run(); // 运行定时器
    }
    callingExpiredTimers_ = false; // 调用到期的定时器结束
    reset(expired, now);           // 重置定时器
}

TimerQueue::TimerQueue(EventLoop *loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timerfdChannel_(loop, timerfd_),
      timers_(),
      callingExpiredTimers_(false)
{
    // we are always reading the timerfd, we disarm it with timerfd_settime.
    // 我们总是读取timerfd，我们使用timerfd_settime来解除武装。
    timerfdChannel_.setReadCallBack(std::bind(&TimerQueue::handleRead, this));
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);

    //
    for (const Entry &timer : timers_)
    {
        // 释放 Timer 对象
        delete timer.second;
    }
}

TimerId TimerQueue::addTimer(const TimerCallback &cb, Timestamp when, double interval)
{
    Timer *timer = new Timer(cb, when, interval);                          // 创建定时器
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer)); // 在 IO 线程中添加定时器
    return TimerId(timer, timer->sequence());                              // 返回定时器 ID
}

void TimerQueue::cancel(TimerId timerId)
{
    loop_->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerId)); // 在 IO 线程中取消定时器
}
