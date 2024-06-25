#include "Socket.h"
#include "Logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <strings.h>
#include <unistd.h>

Socket::Socket(int sockfd) : sockfd_(sockfd)
{
}

Socket::~Socket()
{
    close(sockfd_);
}

/**
 * @brief 绑定地址

*/
void Socket::bindAddress(const InetAddr &localaddr)
{
    if (0 != ::bind(sockfd_, (sockaddr *)localaddr.getSockaddr(), sizeof(sockaddr_in)))
    {
        LOG_FATAL("bind sockfd:%d fail \n", sockfd_);
    }
}

/**
 * @brief 开启监听套接字变化
 */
void Socket::listen()
{
    if (0 != ::listen(sockfd_, 1024))
    {
        LOG_FATAL("listen sockfd:%d fail \n", sockfd_);
    }
}

/**
 * @brief 提取已连接套接字、并将对端信息填充到peeraddr中
 */
int Socket::accept(InetAddr *peeraddr)
{
    sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    bzero(&addr, sizeof(addr));

    int connfd = ::accept(sockfd_, (sockaddr *)&addr, &addrlen);
    if (connfd >= 0)
    {
        peeraddr->setSockaddr(addr);
    }
    else
    {
        LOG_ERROR("accept sockfd:%d fail \n", sockfd_);
    }

    return connfd;
}

// 用于设置TCP连接是否使用Nagle算法，Nagle算法是一种改善网络传输性能的算法。
void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(sizeof optval));
}

// reuseaddr: 允许重用本地地址
void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof optval));
}

// reuseport: 允许重用本地端口
void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, static_cast<socklen_t>(sizeof optval));
}

// 通过设置SO_KEEPALIVE选项，可以让操作系统检测死连接。
void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof optval));
}

void Socket::shutdownWrite()
{
    // 关闭写端-进入写半关闭状态--本端不再发送数据，但是可以接收数据。
    // 1. 对端接收到FIN后，会回复一个ACK，然后进入CLOSE_WAIT状态。
    if (::shutdown(sockfd_, SHUT_WR) < 0)
    {
        LOG_ERROR("shutdownWrite error");
    }
}
