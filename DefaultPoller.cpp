
#include "Poller.h"
#include "EpollPoller.h"

#include <stdlib.h>

/**
 * @brief eventloop可以通过该接口获取默认的IO复用的具体实现,默认使用EpollPoller
 */
Poller *Poller::newDefaultPolle(EventLoop *loop)
{
    if (::getenv("MUDOU_USE_POLL"))
    {
        return nullptr; // 生成pollpoller的实例
    }
    else
    {
        return new EpollPoller(loop); // 生成epoll的实例
    }
}
