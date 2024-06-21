#pragma once
#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread
{
    //__thread: 这是一个线程局部存储（TLS）的修饰符，在多线程环境中，每个线程都会拥有一个独立的变量副本，因此可以保证线程安全性
    // 参考Java的ThreadLocal<T>
    extern __thread int t_cachedTid;
    void cacheTid();

    inline int tid()
    {
        // 执行概率较小
        if (__builtin_expect(t_cachedTid == 0, 0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }

} // namespace CurrentThread
