#include "TcpServer.h"
#include "Logger.h"
#include "TcpConnection.h"

#include <strings.h>
#include <functional>

static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

/**
 * @param option 创建TcpServer的选项、是否重用端口
 * @param loop mainLoop(mainReactor)对象
 */
TcpServer::TcpServer(EventLoop *loop, const InetAddr &listenAddr, const std::string &nameArg, Option option)
    : loop_(CheckLoopNotNull(loop)),
      ipPort_(listenAddr.toIpPort()),
      name_(nameArg),
      acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
      threadPool_(new EventLoopThreadPool(loop, name_)),
      connectionCallback_(),
      messageCallback_(),
      nextConnId_(1),
      started_(0)
{
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    for (auto &item : connections_)
    {
        // 这个局部的shared_ptr智能指针对象，出右括号，可以自动释放new出来的TcpConnection对象资源了
        TcpConnectionPtr conn(item.second);
        item.second.reset(); // 释放该智能指针所持有的对象--释放TcpConnection对象
        // 销毁连接
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

/**
 * @brief 设置底层subloop的个数
 */
void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

/**
 * @brief 开启最上层服务器监听.
 * 1. 启动IO线程池
 * 2. 启动服务线程-mainLoop监听新连接
 */
void TcpServer::start()
{
    if (started_++ == 0) // 防止一个TcpServer对象被start多次
    {
        // 启动IO线程池
        threadPool_->start(threadInitCallback_);
        // 启动服务线程-mainLoop监听新连接
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get())); // 启动最上层的loop监听新客户端连接
    }
}

/**
 * @brief 新连接到来时的回调函数
 */
void TcpServer::newConnection(int sockfd, const InetAddr &peerAddr)
{
    // 轮询算法，选择一个subLoop(IO线程)，来管理channel
    EventLoop *ioLoop = threadPool_->getNextLoop();

    // 创建TcpConnection对象
    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s \n",
             name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

        // 通过sockfd获取其绑定的本机的ip地址和端口信息
    sockaddr_in local;
    ::bzero(&local, sizeof local);
    socklen_t addrlen = sizeof local;

    // 获取本地的ip和端口
    // 如果调用成功，local 变量会被填充为套接字的本地地址信息，addrlen 会被更新为地址信息的实际长度
    if (::getsockname(sockfd, (sockaddr *)&local, &addrlen) < 0)
    {
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddr localAddr(local);

    // 根据连接成功的sockfd，创建TcpConnection连接对象
    TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
    connections_[connName] = conn;

    // 设置连接的各种回调函数
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    // 设置了如何关闭连接的回调   conn->shutDown()
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));

    // 建立连接
    //  直接调用TcpConnection::connectEstablished、
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

/**
 * @brief 删除连接.
 */
void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

/**
 * @brief 在连接所属的loop中删除连接.
 */
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n",
             name_.c_str(), conn->name().c_str());
    connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getLoop();

    // 放到连接所属的loop中执行回调
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn));
}
