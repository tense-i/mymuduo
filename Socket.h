#pragma once

#include "noncopyable.h"
#include "InetAddr.h"

// q: Socket类的作用是什么？
// a: Socket类的作用是封装socket的创建、绑定、监听、连接、关闭等操作。
class Socket : noncopyable
{
private:
    const int sockfd_;

public:
    explicit Socket(int sockfd);
    ~Socket();

    int fd() const { return sockfd_; }

    void bindAddress(const InetAddr &localaddr);

    void listen();

    /**
     * @brief 提取已连接套接字、并将对端信息填充到peeraddr中
     */
    int accept(InetAddr *peeraddr);

    // 用于设置TCP连接是否使用Nagle算法，Nagle算法是一种改善网络传输性能的算法。
    void setTcpNoDelay(bool on);
    // reuseaddr: 允许重用本地地址
    void setReuseAddr(bool on);
    // reuseport: 允许重用本地端口
    void setReusePort(bool on);
    // 通过设置SO_KEEPALIVE选项，可以让操作系统检测死连接。
    void setKeepAlive(bool on);

    void shutdownWrite();

    static int getSocketError(int sockfd);       // 获取socket错误
    static int createNonblockingFd();            // 创建一个非阻塞的socket
    static bool isSelfConnect(int sockfd);       // 判断是否是自连接
    static sockaddr_in getLocalAddr(int sockfd); // 获取本地地址
    static sockaddr_in getPeerAddr(int sockfd);  // 获取对端地址
};
