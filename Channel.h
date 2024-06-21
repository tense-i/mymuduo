#pragma once
#include "noncopyable.h"
#include <functional>
#include "Timestamp.h"
#include <memory>

class EventLoop;

/**
 * @brief Channel理解为通道、处理socketfd的事件分发.但是**Channel并不负责fd的生命周期**（P281, 8.1.1节），fd的生命周期是交给Socket类来管理的.
 * @brief Channel与fd_是聚合关系-生命周期独立
 */
class Channel : noncopyable
{
public:
    using EventCB = std::function<void()>;
    using ReadEventCB = std::function<void(Timestamp)>;
    Channel(EventLoop *loop, int fd);
    ~Channel();

private:
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;
    EventLoop *loop_; // 并发下标识该Channel属于哪个事件循环
    const int fd_;    // fd poller监听的对象
    int events_;      // 注册fd感兴趣的事件
    int revents_;     // poller返回的具体发生的事件
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_;

    // 因为channel能够知道具体发生的事件、channel负责通过回调函数、调用具体的事件处理函数
    ReadEventCB readCallBack_;
    EventCB writeCallBack_;
    EventCB closeCallBack_;
    EventCB errorCallBack_;

public:
    /**
     * @brief fd触发的事件、由handler分发.fd得到poller通知以后，处理事件的
     */
    void handleEvent(Timestamp receiveTime);

    void setReadCallBack(ReadEventCB cb)
    {
        readCallBack_ = std::move(cb);
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
    int revents();
    void setRevent(int revents);

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
    void set_index(int idx);

    // one loop per thread
    EventLoop *ownerLoop();
    void remove();

private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

public:
};
