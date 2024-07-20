#pragma once
#include "noncopyable.h"
#include <memory>
#include "InetAddr.h"

#include "CallBacks.h"
#include <string>
#include <mutex>

class Connector;
class EventLoop;

class TcpClient : noncopyable
{

public:
    using ConnectorPtr = std::shared_ptr<Connector>;
    TcpClient(EventLoop *loop, const InetAddr &serverAddr, const std::string &name);
    ~TcpClient();

    void connect();
    void disconnect();
    void stop();

    TcpConnectionPtr connection() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return connection_;
    }
    EventLoop *getLoop() const { return loop_; }
    bool retry() const { return retry_; }
    void enableRetry()
    {
        retry_ = true;
    }
    const std::string &name() const
    {
        return name_;
    }

    void setConnectionCallback(ConnectionCallback cb)
    {
        connectionCallback_ = std::move(cb);
    }
    void setMessageCallback(MessageCallback cb)
    {
        messageCallback_ = std::move(cb);
    }
    void setWriteCompleteCallback(WriteCompleteCallback cb)
    {
        writeCompleteCallback_ = std::move(cb);
    }

private:
    void
    newConnection(int sockfd);                           // 新连接处理函数
    void removeConnection(const TcpConnectionPtr &conn); // 移除连接
    void removeConnectionInLoop(EventLoop *loop, const TcpConnectionPtr &conn);
    void TcpClient::removeConnctor(const ConnectorPtr &connector);

private:
    EventLoop *loop_;        // 事件循环
    ConnectorPtr connector_; // 连接器
    const std::string name_;
    ConnectionCallback connectionCallback_;       // 连接回调
    MessageCallback messageCallback_;             // 消息回调
    WriteCompleteCallback writeCompleteCallback_; // 写完成回调
    bool retry_;                                  // 是否重连
    bool connect_;                                // 是否连接
    int nextConnId_;                              // 下一个连接ID
    mutable std::mutex mutex_;                    // 互斥锁
    TcpConnectionPtr connection_;                 // 连接
};
