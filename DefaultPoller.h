#include "Poller.h"
#include <stdlib.h>

Poller *Poller::newDefaultPolle(EventLoop *loop)
{
    if (::getenv("MUDOU_USE_POLL"))
    {
        return nullptr; // 生成poll的实例
    }
    else
    {
        return nullptr; // 生成epoll的实例
    }
    return nullptr;
}
