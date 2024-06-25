#pragma once
#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddr.h"
#include "CallBacks.h"

#include "noncopyable.h"
#include "EventLoopThreadPool.h"

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>

/**
 * @brief 对外提供的TcpServer类、用于上层网络编程中的服务器端
 */
class TcpServer : noncopyable
{
public:
    using ThreadInitCallBack = std::function<void(EventLoop *)>;

    /**
     * @brief Option枚举类的作用是设置是否重用端口
     */
    enum Option
    {
        kNoReusePort,
        kReusePort,
    };
    TcpServer(EventLoop *loop, const InetAddr &listenAddr, const std::string &nameArg, Option option = kNoReusePort);

    ~TcpServer();

    // 设置回调函数

    void setThreadInitcallback(const ThreadInitCallBack &cb) { threadInitCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

    void setThreadNum(int numThreads); // 设置线程池的线程数量

    void start(); // 开启服务器监听

private:
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;
    EventLoop *loop_; // 服务器监听的EventLoop-mainLoop
    const std::string ipPort_;
    const std::string name_;

    std::unique_ptr<Acceptor> acceptor_; // 运行在mainLoop，任务就是监听新连接事件

    std::shared_ptr<EventLoopThreadPool> threadPool_; // 线程池，用于处理新连接的读写事件one loop per thread

    ConnectionCallback connectionCallback_; // 有新连接时的回调函数

    MessageCallback messageCallback_; // 有读写事件时的回调函数

    WriteCompleteCallback writeCompleteCallback_; // 有写事件时的回调函数

    ThreadInitCallBack threadInitCallback_; // 线程初始化回调函数

    std::atomic_int started_;
    int nextConnId_; // 下一个连接的id

    ConnectionMap connections_; // 存放所有的连接

private:
    void newConnection(int sockfd, const InetAddr &peerAddr);  // 新连接到来时的回调函数
    void removeConnection(const TcpConnectionPtr &conn);       // 删除连接
    void removeConnectionInLoop(const TcpConnectionPtr &conn); // 在loop中删除连接
};
