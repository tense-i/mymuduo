// CurrentThread.cpp
#include "CurrentThread.h"
#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread
{
    __thread int t_cachedTid = 0; // 定义全局变量
    /**
     * @brief 缓存线程id

    */
    void cacheTid()
    {
        if (t_cachedTid == 0)
        {
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}
