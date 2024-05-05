#include "Channel.h"
#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

/**
 * @param loop loop中包含了许多的channel（fd）poller
 */
Channel::Channel(EventLoop *loop, int fd) : loop_(loop), fd_(fd), events_(kNoneEvent), revents_(kNoneEvent), index_(-1), tied_(false)
{
}

Channel::~Channel()
{
}

bool Channel::isNoneEvent() const
{
    return events_ == kNoneEvent;
}

bool Channel::isWriting() const
{
    return events_ & kWriteEvent;
}

bool Channel::isReading() const
{
    return events_ & kReadEvent;
}

int Channel::index()
{
    return index_;
}

void Channel::set_index(int idx)
{
    index_ = idx;
}

EventLoop *Channel::ownerLoop()
{
    return loop_;
}

// 在channel所属的eventloop中、把当前的channel删除
void Channel::remove()
{
    // redo add code...
    //  loop_->removeChannel(this);
}

/**
 * @brief 改变channel所表示的fd的事件后、updateEpoll负责再poller里面更改fd相应的Epoll节点
 */
void Channel::updateEpoll()
{
    // 通过channel所属的eventloop、调用poller的相应方法、注册fd的events事件
    // loop_->updateChannel(this);
}

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) // 客户端连接断开
    {
        if (closeCallBack_)
            closeCallBack_();
    }
    else if ((revents_ & EPOLLIN)) //
    {
        if (readCallBack_)
            readCallBack_(receiveTime);
    }
    else if ((revents_ & EPOLLOUT))
    {
        if (writeCallBack_)
            writeCallBack_();
    }
    else if ((revents_ & EPOLLERR))
    {
        if (errorCallBack_)
            errorCallBack_();
    }
}

/**
 * @brief 绑定一个弱指针作为监视者
 */
void Channel::handleEvent(Timestamp receiveTime)
{
    if (tied_)
    {
        // 提升弱指针
        std::shared_ptr<void> guard = tie_.lock();
        if (guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

void Channel::enableReading()
{
    events_ |= kReadEvent;
    updateEpoll();
}

void Channel::disableReading()
{
    events_ &= ~kReadEvent;
    updateEpoll();
}

void Channel::enableWriting()
{
    events_ |= kWriteEvent;
    updateEpoll();
}

void Channel::disableWriting()
{
    events_ &= ~kWriteEvent;
    updateEpoll();
}

void Channel::disableAll()
{
    events_ = kNoneEvent;
    updateEpoll();
}
