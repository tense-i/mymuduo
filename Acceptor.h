#pragma once

#include "noncopyable.h"
#include "Channel.h"
#include <functional>
#include "Socket.h"

class EventLoop;
class InetAddress;
// Acceptor类的作用是封装accept函数，用于接受新的连接
class Acceptor : noncopyable
{
public:
    using NewConnectionCB = std::function<void(int sockfd, const InetAddr &)>;

    Acceptor(EventLoop *loop, const InetAddr &listenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCB &cb)
    {
        newConnectionCallback_ = cb;
    }

    bool listenning() const { return listenning_; }
    void listen();

private:
    void handleRead();
    // Acceptor用的就是用户定义的那个baseLoop，也称作mainLoop
    EventLoop *loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;

    /**
     * @brief 有新的连接到来时的回调函数
     *  该回调函数会在TcpServer::newConnection中被设置-用于打包新连接的Channel、封装到subLoop中（fd-->channel)
     */
    NewConnectionCB newConnectionCallback_;
    bool listenning_;
};
