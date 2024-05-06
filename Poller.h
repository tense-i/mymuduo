#pragma once
#include "noncopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

/**
 * @brief muduo库中多路事件分发器的核心IO复用模块
 */
class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel *>;

    // key:sockfd value: Channel*
    using ChannelMap = std::unordered_map<int, Channel *>;

protected:
    ChannelMap Channels_;

private:
    EventLoop *ownerLoop_; // 定义Poller所属的事件循环

public:
    Poller(EventLoop *loop);
    virtual ~Poller() = default;
    /**
     * @brief 给所有IO复用保留同一的接口--纯虚函数
     */
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannle(Channel *channel) = 0;

    /**
     * @brief 判断参数channel是否在当前的Poller中
     */
    bool hasChannel(Channel *channel) const;

    /**
     * @brief eventloop可以通过该接口获取默认的IO复用的具体实现
     */
    static Poller *newDefaultPolle(EventLoop *loop);
};
