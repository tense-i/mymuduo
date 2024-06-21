#include "CurrentThread.h"

namespace CurrentThread
{
    __thread int t_cachedTid = 0;
    void cacheTid() // Remove explicit qualification "CurrentThread::"
    {
        if (t_cachedTid == 0)
        {
            t_cachedTid == static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
} // namespace CurrentThread
