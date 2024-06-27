#include "Channel.h"
#include "Logger.h"
#include "EventLoop.h"
#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

/**
 * @param loop 该channel所属的eventloop
 */
Channel::Channel(EventLoop *loop, int fd) : loop_(loop), fd_(fd), events_(kNoneEvent), revents_(kNoneEvent), index_(-1), tied_(false)
{
}

Channel::~Channel()
{
}

/**
 * @brief 该channel是否是未监视任何事件
 */
bool Channel::isNoneEvent() const
{
    return events_ == kNoneEvent;
}

/**
 * @brief 该channel的fd是否监听写事件
 */
bool Channel::isWriting() const
{
    return events_ & kWriteEvent;
}

/**
 * @brief 该channel的fd是否监听读事件
 */
bool Channel::isReading() const
{
    return events_ & kReadEvent;
}

/**
 * @brief 表示当前channel在EpollPoller中的状态。取值为kNew、kAdded kDeleted
 * kNew:没有进入EpollPoller
 * kAdded:进入过EpollPoller
 */
int Channel::index()
{
    return index_;
}

/**
 * @brief 设置channel在EpollPoller中的状态

*/
void Channel::setIndex(int idx)
{
    index_ = idx;
}

/**
 * @brief 定义EPoller所属的事件循环EventLoop
 */
EventLoop *Channel::ownerLoop()
{
    return loop_;
}

/**
 * @brief 在channel所属的eventloop中、把当前的channel删除
 */

void Channel::remove()
{
    loop_->removeChannel(this);
}

/**
 * @brief 在channel所属的eventloop中、更新channel的fd的events事件.
 */
void Channel::update()
{
    // 通过channel所属的eventloop、调用poller的相应方法、注册fd的events事件
    loop_->updateChannel(this);
}

/**
 * @brief 根据poller通知的channel发生的具体事件， 由channel负责调用具体的回调操作
 * @param receiveTime epoll_wait返回的时间戳。
 */
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("channel handleEvent revents:%d\n", revents_);
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) // 客户端连接断开、closeCallBack_会将channel从poller中删除
    {
        if (closeCallBack_)
            closeCallBack_();
    }

    if ((revents_ & EPOLLERR)) // 错误
    {
        if (errorCallBack_)
            errorCallBack_();
    }

    if (revents_ & (EPOLLIN | EPOLLPRI)) // 读事件
    {
        if (readCallBack_)
            readCallBack_(receiveTime);
    }

    if ((revents_ & EPOLLOUT)) // 写事件
    {
        if (writeCallBack_)
            writeCallBack_();
    }
}

/**
 * @brief fd得到poller通知以后，处理事件的分发处理
 * @param receiveTime 事件发生的时间
 */
void Channel::handleEvent(Timestamp receiveTime)
{
    if (tied_) // 防止channel在客户端主动remove掉、channel还在执行回调操作。具体看笔记Channel的tie_涉及到的精妙之处
    {
        // 提升弱指针
        std::shared_ptr<void> guard = tie_.lock();
        if (guard) // 说明channel还在
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else // channel已经被remove掉了
    {
        handleEventWithGuard(receiveTime);
    }
}
/**
 * @brief 防止当channel被手动remove掉、channel还在执行回调操作
 */
void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

int Channel::fd()
{
    return this->fd_;
}

int Channel::events()
{
    return events_;
}

void Channel::setRevents(int revts)
{
    revents_ = revts;
}

/**
 * @brief 使能读事件

*/
void Channel::enableReading()
{
    events_ |= kReadEvent;
    update(); // 更新channel的fd的events事件
}

void Channel::disableReading()
{
    events_ &= ~kReadEvent;
    update();
}

void Channel::enableWriting()
{
    events_ |= kWriteEvent;
    update();
}

void Channel::disableWriting()
{
    events_ &= ~kWriteEvent;
    update();
}

void Channel::disableAll()
{
    events_ = kNoneEvent;
    update();
}
