#include "Channel.h"
#include "Logger.h"

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

void Channel::set_index(int idx)
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
    // redo add code...
    //  loop_->removeChannel(this);
}

/**
 * @brief 当改变channel所表示fd的events事件后，update负责在poller里面更改fd相应的事件epoll_ctl
 * EventLoop => ChannelList   Poller
 */
void Channel::update()
{
    // 通过channel所属的eventloop、调用poller的相应方法、注册fd的events事件
    // loop_->updateChannel(this);
}

/**
 * @brief 根据poller通知的channel发生的具体事件， 由channel负责调用具体的回调操作
 * @param receiveTime epoll_wait返回的时间戳。
 */
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("channel handleEvent revents:%d\n", revents_);
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) // 客户端连接断开
    {
        if (closeCallBack_)
            closeCallBack_();
    }
    else if (revents_ & (EPOLLIN | EPOLLPRI)) // 读
    {
        if (readCallBack_)
            readCallBack_(receiveTime);
    }
    else if ((revents_ & EPOLLOUT)) // 写
    {
        if (writeCallBack_)
            writeCallBack_();
    }
    else if ((revents_ & EPOLLERR)) // 错误
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

int Channel::fd()
{
    return this->fd_;
}

int Channel::events()
{
    return events_;
}

int Channel::revents()
{
    return revents_;
}

void Channel::setRevent(int revents)
{
    revents_ = revents;
}

void Channel::enableReading()
{
    events_ |= kReadEvent;
    update();
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
