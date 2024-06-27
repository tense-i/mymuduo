#pragma once
#include "noncopyable.h"
#include <functional>
#include "Timestamp.h"
#include <memory>

class EventLoop;

/**
 * @brief Channel类是一个fd的事件分发器。负责fd的事件分发、事件处理。
 * @brief Channel与fd_是聚合关系-生命周期独立
 */
class Channel : noncopyable
{
public:
    using EventCB = std::function<void()>;              // 其它事件回调函数
    using ReadEventCB = std::function<void(Timestamp)>; // 读事件回调函数
    Channel(EventLoop *loop, int fd);
    ~Channel();

    /**
     * @brief fd得到poller通知以后，处理事件的分发处理
     * @param receiveTime 事件发生的时间
     */
    void handleEvent(Timestamp receiveTime);

    void setReadCallBack(ReadEventCB cb)
    {
        readCallBack_ = std::move(cb); // 为什么要用std::move()？
        // a:因为readCallBack_是一个右值引用，std::move()将左值转换为右值引用，避免拷贝构造函数的调用。
        // 其实可以直接赋值，但是为了提高效率，使用std::move()。
    }

    void setWriteCallBack(EventCB cb)
    {
        writeCallBack_ = std::move(cb);
    }
    void setCloseCallBack(EventCB cb)
    {
        closeCallBack_ = std::move(cb);
    }
    void setErrorCallBack(EventCB cb)
    {
        errorCallBack_ = std::move(cb);
    }

    /**
     * @brief 防止当channel被手动remove掉、channel还在执行回调操作
     */
    void tie(const std::shared_ptr<void> &obj);

    int fd();
    int events();
    void setRevents(int revents);

    // 设置fd对应的事件使能
    void enableReading();
    void disableReading();
    void enableWriting();
    void disableWriting();
    void disableAll();

    // 返回fd当前的事件状态
    bool isNoneEvent() const;
    bool isWriting() const;
    bool isReading() const;

    int index();
    void setIndex(int idx);

    // one loop per thread
    EventLoop *ownerLoop();
    void remove();

private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;
    EventLoop *loop_; // 并发下、标识该Channel属于哪个事件循环
    const int fd_;    // fd poller监听的对象
    int events_;      // 注册fd感兴趣的事件
    int revents_;     // poller返回的具体发生的事件
    int index_;       // 用于标识channel在poller中的状态,取值为kNew、kAdded、kDeleted

    // weak_ptr实现观察者模式
    std::weak_ptr<void> tie_; // 用于解决channel的生命周期问题
    bool tied_;

    // 因为channel能够知道具体发生的事件、channel负责通过回调函数、调用具体的事件处理函数
    ReadEventCB readCallBack_;
    EventCB writeCallBack_;
    EventCB closeCallBack_;
    EventCB errorCallBack_;
};
