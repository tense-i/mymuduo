#pragma once

namespace CurrentThread
{
    //__thread: 这是一个线程局部存储（TLS）的修饰符，在多线程环境中，每个线程都会拥有一个独立的变量副本，因此可以保证线程安全性
    // 参考Java的ThreadLocal<T>
    extern __thread int t_cachedTid; // 声明变量
    void cacheTid();                 // 声明函数

    inline int tid()
    {
        // __builtin_expect是GCC内建函数，作用是告诉编译器，分支条件的可能性，以便编译器生成更好的代码。
        if (__builtin_expect(t_cachedTid == 0, 0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}