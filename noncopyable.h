#pragma once

/**
 * @param noncopyable被继承后、派生类对象可以正常构造、析构，但是不可拷贝构造与拷贝赋值
 */
class noncopyable
{
protected:
    noncopyable() = default;
    ~noncopyable() = default;

public:
    // 将拷贝构造函数与拷贝赋值函数定义为删除函数、禁止拷贝构造与拷贝赋值
    noncopyable(const noncopyable &) = delete;
    noncopyable &operator=(const noncopyable &) = delete;
};
