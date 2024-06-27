#pragma once
#include "noncopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

/**
 * @brief muduo库中多路事件分发器的核心IO复用模块，被泛化为PollPoller类(poll)和EpollPoller类(epoll)
 */
class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel *>;
    using ChannelMap = std::unordered_map<int, Channel *>; // fd和channel的映射关系

protected:
    ChannelMap Channels_; // Poller中的CHannel列表，保存fd和channel的映射关系,用于快速查找fd对应的channel.

private:
    EventLoop *ownerLoop_; // 定义Poller所属的事件循环类对象

public:
    Poller(EventLoop *loop);
    virtual ~Poller() = default;

    // 给所有IO复用保留统一的接口
    /**
     * @brief 开启底层事件循环、等待事件的发生
     */
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;

    /**
     * @brief 更新channel管理的fd对应的事件
     */
    virtual void updateChannel(Channel *channel) = 0;

    /**
     * @brief 从poller中删除Channel
     */
    virtual void removeChannle(Channel *channel) = 0;

    /**
     * @brief 判断参数channel是否在当前的Poller中
     */
    bool hasChannel(Channel *channel) const;

    /**
     * @brief eventloop可以通过该接口获取默认的IO复用的具体实现,默认使用EpollPoller
     */
    static Poller *newDefaultPolle(EventLoop *loop);
};
