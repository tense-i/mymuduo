#pragma once

#include "noncopyable.h"
#include "InetAddr.h"
#include "CallBacks.h"
#include "Buffer.h"
#include "Timestamp.h"

#include <memory>
#include <string>
#include <atomic>
class Channel;
class EventLoop;
class Socket;

class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
private:
    enum StateE
    {
        kDisconnected,
        kConnecting,
        kConnected,
        kDisconnecting
    };

    EventLoop *loop_; // 这里绝对不是baseLoop， 因为TcpConnection都是在subLoop里面管理的

    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    // Acceptor属于mainLoop    TcpConenction属于subLoop
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddr localAddr_;
    const InetAddr peerAddr_;

    ConnectionCallback connectionCallback_;       // 有新连接时的回调
    MessageCallback messageCallback_;             // 有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_; // 消息发送完成以后的回调
    HighWaterMarkCallback highWaterMarkCallback_; // 高水位回调
    CloseCallback closeCallback_;

    size_t highWaterMark_; // 高水位标记

    Buffer inputBuffer_;  // 接收数据的缓冲区
    Buffer outputBuffer_; // 发送数据的缓冲区
public:
    TcpConnection(EventLoop *loop, const std::string &name, int sockfd, const InetAddr &localAddr, const InetAddr &peerAddr);
    ~TcpConnection();

public:
    EventLoop *getLoop() const { return loop_; }
    const std::string &name() const { return name_; }
    const InetAddr &localAddr() const { return localAddr_; }
    const InetAddr &peerAddr() const { return peerAddr_; }

    bool connected() const
    {
        return state_ == kConnected;
    }

    void send(const std::string &msg);

    void shutdown();

    void connectEstablished();
    void connectDestroyed();

    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }
    void setHighWaterMarkCallBack(const HighWaterMarkCallback &cb, size_t highWaterMark)
    {
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }
    void setCloseCallback(const CloseCallback &cb) { closeCallback_ = cb; }

private:
    void setState(StateE state) { state_ = state; }

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void *message, size_t len);
    void shutdownInLoop();
};
