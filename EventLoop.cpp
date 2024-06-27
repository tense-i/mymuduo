#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory>
#include <error.h>

//// 防止一个线程创建多个EventLoop   thread_local
// 只需要在子线程执行函数（即）中构造一个EventLoop对象，EventLoop的构造函数会把`t_loopInThisThread`初始化为this，这样一来，如果该线程中再次创建一个EventLoop对象时，就会在其构造函数中终止程序
// 会在线程被创建时、初始化t_loopInthisThread
__thread EventLoop *t_loopInthisThread = nullptr;

// 定义默认的Poller IO复用接口的超时时间
const int KPollTimeMs = 10000;

int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL("eventfd error:%d \n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop() : looping_(false),
                         quit_(false), callingPendingFunctors_(false),
                         threadId_(CurrentThread::tid()),
                         poller_(Poller::newDefaultPolle(this)),
                         wakeupfd_(createEventfd()),
                         wakeupChannel_(new Channel(this, wakeupfd_))

{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);

    // 执行构造EventLoop的时候、t_loopInthisThread已经有值。意味着这个EventLoop已经有对应的、存在的线程
    if (t_loopInthisThread) // 多次start
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInthisThread, threadId_);
    }
    else
    {
        t_loopInthisThread = this; // 初始化t_loopInthisThread
    }

    // 设置wakeupfd的事件类型以及发生事件后的回调操作
    wakeupChannel_->setReadCallBack(std::bind(&EventLoop::handleRead, this));
    // 每一个eventloop都将监听wakeupchannel的EPOLLIN读事件了
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupfd_);
    t_loopInthisThread = nullptr;
}

/**
 * @brief 开启上层事件循环
 */
void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);

    while (!quit_)
    {
        activeChannels_.clear();
        // 监听两类fd   一种是client的fd，一种wakeupfd。
        // 在这里会阻塞、调用了epoll_wait
        pollReturnTime_ = poller_->poll(KPollTimeMs, &activeChannels_);

        // 到这里，poller就已经返回了，说明有事件发生了
        for (Channel *channel : activeChannels_)
        {
            // Poller监听哪些channel发生事件了，然后上报给EventLoop，通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }

        // Poller中事件发生后、执行当前EventLoop事件循环需要处理的回调操作
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping \n", this);
    looping_ = false;
}

/**
 * @brief 退出事件循环。有两种被调用的情况
 * 1.loop在自己的线程中调用quit  2.在非loop的线程中，调用loop的quit
 */
void EventLoop::quit()
{
    quit_ = true;

    // 如果是在其它线程中，调用的quit   在一个subloop(woker)中，调用了mainLoop(IO)的quit
    // 不是在自己线程中调用quit、唤醒自己的线程执行quit操作
    if (!isInLoopThread())
    {
        wakeup(); // 唤醒loop所在的线程、执行退出
    }
}

/**
 * @brief 返回poller返回的时间戳

*/
Timestamp EventLoop::pollReturnTime() const
{
    return pollReturnTime_;
}

/**
 * @brief 在当前的loop中执行cb
 */
void EventLoop::runInLoop(Functor cb)
{
    // q:为什么要分开处理？
    // 保证一个fd只在一个线程中被处理
    if (isInLoopThread()) // 在当前的loop线程中，执行cb
    {
        cb();
    }
    else // 当前loop所在线程和cb所在线程不是同一个线程，不应该在当前loop线程中执行cb
    {
        // 保存到队列中，唤醒loop所在线程，执行cb
        queueInLoop(cb);
    }
}

/**
 * @brief 把cb放入队列中，唤醒loop所在的线程，执行cb
 */
void EventLoop::queueInLoop(Functor cb)
{
    {
        // 为什么要加锁？？
        // 因为有可能在多个线程中调用queueInLoop()函数，向pendingFunctors_中添加回调函数
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }
    // 唤醒相应的，需要执行上面回调操作的loop的线程了
    // || callingPendingFunctors_的意思是：当前loop正在执行回调，但是loop又有了新的回调
    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup(); // 唤醒loop所在线程
    }
}

/**
 *@brief 用来唤醒loop所在的线程的  向wakeupfd_写一个数据，wakeupChannel就发生读事件，当前loop线程就会被唤醒---》执行回调。EventLoop::loop()中pendingFunctors_回调是在事件被触发后执行的
 */
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupfd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
    // 写入数据后、EventLoop::loop()便不会因为没有事件而阻塞
    // 会立即返回，执行doPendingFunctors()，执行回调
}

/**
 * @brief 注册或更新channel。
 * 搭建了poller与channel交流的平台
 */
void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}

/**
 * @brief 从poller中删除channel

*/
void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannle(channel);
}

/**
 * @brief 判断channel是否在poller中

*/
bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}

//
/**
 * @brief 判断EventLoop对象是否在自己的线程里面
 */
bool EventLoop::isInLoopThread() const
{
    // CurrentThread::tid()返回当前线程的tid
    // 在每个线程初始化时、已经缓存了当前线程的tid（CurrentThread::t_cachedTid）
    // 如果当前loop所在线程的tid和当前线程的tid相等，说明当前loop对象在当前线程中
    return threadId_ == CurrentThread::tid();
}

/**
 * @brief 执行事件的回调函数。
 */
void EventLoop::doPendingFunctors() // 执行回调
{
    // 为什么要用临时数组保存？
    // 在下面foreach执行过程中、有可能其它线程在往pendingFunctors_中插入回调函数。所以要加锁
    std::vector<Functor> functors;
    // q:为什么要用一个临时变量来存储回调函数？
    // a:因为在执行回调函数的过程中，有可能会再次调用queueInLoop()函数，从而向pendingFunctors_中添加新的回调函数。

    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_); // 将待处理的回调操作交换到临时容器中，并清空原始容器
    }

    for (const Functor &functor : functors)
    {
        functor(); // 执行当前loop需要执行的回调操作
    }

    callingPendingFunctors_ = false;
}

/**
 * @brief wakeupfd_的读事件回调函数
 */
void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupfd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8", n);
    }
}