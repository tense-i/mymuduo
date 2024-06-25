#pragma once
#include "Poller.h"
#include <vector>
#include <sys/epoll.h>

/**
 * @brief EpollPoller继承自Poller、封装的是Epoll的使用。
 */
class EpollPoller : public Poller
{

public:
    EpollPoller(EventLoop *loop);
    ~EpollPoller() override;

private:
    using EventList = std::vector<epoll_event>;
    // epoll句柄
    int epollfd_;
    // epoll的事件数组
    EventList events_;
    // 初始化事件数组的长度
    static const int kInitEventListSize = 16;

private:
    // 填写活跃连接
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    // 更新channel
    void update(int operation, Channel *channel);

public:
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannle(Channel *channel) override;
};
