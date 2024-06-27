
#include "EpollPoller.h"
#include "Logger.h"
#include "Channel.h"
#include <string.h>
#include <errno.h>
#include <unistd.h>

// channel未添加到poller中
const int kNew = -1; // channel的成员index_=-1
// channel已经添加到poller中
const int KAdded = 1;
// 从channel中删除poller
const int kDeleted = 2;

EpollPoller::EpollPoller(EventLoop *loop)
    : Poller(loop),
      epollfd_(epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize)
{
    if (epollfd_ < 0)
    {
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}

EpollPoller::~EpollPoller()
{
    ::close(epollfd_);
}

/**
 * @brief 管理的fd对应的事件、上树、修改、删除
 * @param operation EPOLL_CTL_ADD, EPOLL_CTL_MOD, EPOLL_CTL_DEL
 */
void EpollPoller::update(int operation, Channel *channel)
{
    epoll_event event;
    bzero(&event, sizeof event);

    int fd = channel->fd();

    event.events = channel->events();
    event.data.fd = fd;
    event.data.ptr = channel;

    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}
/**
 * @brief 触发事件时、填写活跃的连接
 */
void EpollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for (int i = 0; i < numEvents; i++)
    {
        Channel *chan = static_cast<Channel *>(events_[i].data.ptr);
        if (chan == nullptr)
        {
            LOG_ERROR("events_[%d].data.ptr is NULL \n", i);
            continue;
        }

        chan->setRevents(events_[i].events); // 设置为发生的事件
        // eventLoop就拿到了它的poller给他返回的所有发生事件的channels列表
        activeChannels->push_back(chan);
    }
}

/**
 * @brief 开启事件循环，等待事件发生

*/
Timestamp EpollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    // 实际上应该用LOG_DEBUG输出日志更为合理
    LOG_INFO("func=%s => fd total count:%lu \n", __FUNCTION__, Channels_.size());

    // 在这会阻塞，直到有事件发生
    int readyNumEvent = epoll_wait(epollfd_, &(*events_.begin()), static_cast<int>(events_.size()), timeoutMs);

    // 在这里触发事件了
    int saveErrno = errno;
    Timestamp now(Timestamp::now());
    if (readyNumEvent > 0) // 有事件发生
    {
        LOG_INFO("%d events happend \n", readyNumEvent);
        // 填充活跃的连接
        fillActiveChannels(readyNumEvent, activeChannels);

        // 发生事件的个数大于channels的最大长度--扩容
        if (readyNumEvent == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (readyNumEvent == 0) // 超时
    {
        LOG_DEBUG("%s timeout\n", __FUNCTION__);
    }
    else
    {
        // EINTR 是一个宏，表示 "Interrupted system call"，即系统调用被中断。通常在多线程或者信号处理的情况下，会出现 poll() 被中断的情况。
        if (saveErrno != EINTR)
        {
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() err");
        }
    }

    return now;
}

/**
 * @brief 更新channel管理的fd对应的事件
 */
void EpollPoller::updateChannel(Channel *channel)
{
    const int index = channel->index();
    LOG_INFO("func=%s fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(), channel->events(), index);

    if (index == kNew || index == kDeleted)
    {
        if (index == kNew)
        {
            int fd = channel->fd();
            Channels_[fd] = channel;
        }
        channel->setIndex(KAdded);
        update(EPOLL_CTL_ADD, channel);
    } // channel已经在POLLER注册过
    else
    {
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->setIndex(kDeleted);
        }
        else
            update(EPOLL_CTL_MOD, channel);
    }
}

/**
 * @brief 从poller中删除Channel
 */
void EpollPoller::removeChannle(Channel *channel)
{
    const int fd = channel->fd();
    int index = channel->index();
    Channels_.erase(fd);
    LOG_INFO("func=%s fd=%d ", __FUNCTION__, fd);

    if (index == KAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->setIndex(kNew); // channel置空
}
