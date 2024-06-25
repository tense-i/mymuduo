#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop *loop) : ownerLoop_(loop)
{
}

bool Poller::hasChannel(Channel *channel) const
{
    auto it = Channels_.find(channel->fd());
    return it != Channels_.end() && it->second == channel;
}
