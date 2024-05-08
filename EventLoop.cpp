#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <fcntl.h>

//// 防止一个线程创建多个EventLoop   thread_local
__thread EventLoop *t_loopInthisThread = nullptr;

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
                         quit_(false),
                         callingPendingFunctors_(false),
                         poller_(Poller::newDefaultPolle(this)),
                         wakeupfd_(createEventfd()),
                         wakeupChannel_(new Channel(this, wakeupfd_)),
                         threadId_(CurrentThread::tid())
{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);

    // 执行构造EventLoop的时候、t_loopInthisThread已经有值。意味着这个EventLoop已经有对应的、存在的线程
    if (t_loopInthisThread)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInthisThread, threadId_);
    }
    else
    {
        t_loopInthisThread = this;
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

// 开启事件循环
void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;
    LOG_INFO("");

    while (!quit_)
    {
        activeChannels_.clear();
        // 监听两类fd   一种是client的fd，一种wakeupfd。
        // 在这里会阻塞、调用了epoll_wait
        pollReturnTime_ = poller_->poll(KPollTimeMs, &activeChannels_);

        for (Channel *channel : activeChannels_)
        {
            // Poller监听哪些channel发生事件了，然后上报给EventLoop，通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        // 执行当前EventLoop事件循环需要处理的回调操作
        /**
         * IO线程 mainLoop accept fd《=channel subloop
         * mainLoop 事先注册一个回调cb（需要subloop来执行）    wakeup subloop后，执行下面的方法，执行之前mainloop注册的cb操作
         */
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
        wakeup();
    }
}

Timestamp EventLoop::pollReturnTime() const
{
    return pollReturnTime_;
}

/**
 * @brief 在当前的loop中执行cb
 */
void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread()) // 在当前的loop线程中，执行cb
    {
        cb();
    }
    else // 在非当前loop线程中执行cb , 就需要唤醒loop所在线程，执行cb
    {
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
 *@brief 用来唤醒loop所在的线程的  向wakeupfd_写一个数据，wakeupChannel就发生读事件，当前loop线程就会被唤醒--不会被epoll_wait阻塞
 */
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupfd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

/**
 * @brief 搭建了poller与channel交流的平台
 */
void EventLoop::updatChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannle(channel);
}

void EventLoop::hasChannel(Channel *channel)
{
    poller_->hasChannel(channel);
}

//
/**
 * @brief 判断EventLoop对象是否在自己的线程里面
 */
bool EventLoop::isInLoopThread() const
{
    //
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
    ssize_t n = read(wakeupfd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR("eventloop::handleRead() reads %dbyters instead of 8", n);
    }
}
