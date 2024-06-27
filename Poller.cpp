#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop *loop) : ownerLoop_(loop)
{
}

/**
 * @brief 判断参数channel是否在当前的Poller中
 */
bool Poller::hasChannel(Channel *channel) const
{
    auto it = Channels_.find(channel->fd());
    return it != Channels_.end() && it->second == channel;
}
