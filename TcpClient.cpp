#include "TcpClient.h"
#include "TcpConnection.h"
#include "Connector.h"
#include "Socket.h"
#include "EventLoop.h"
#include "Logger.h"

TcpClient::TcpClient(EventLoop *loop, const InetAddr &serverAddr, const std::string &name)
    : loop_(loop),
      connector_(std::make_shared<Connector>(loop, serverAddr)),
      name_(name),
      connectionCallback_(defaultConnectionCallback), // 默认的连接回调函数
      messageCallback_(defaultMessageCallback),
      writeCompleteCallback_(defaultWriteCompleteCallback),
      retry_(false),
      connect_(true),
      nextConnId_(1)
{
    // 给连接器设置新连接回调函数
    connector_->setNewConnectionCallback(std::bind(&TcpClient::newConnection, this, std::placeholders::_1));
}

void TcpClient::removeConnctor(const ConnectorPtr &connector)
{

    // undo
}
TcpClient::~TcpClient()
{
    LOG_INFO("TcpClient::~TcpClient[%s] - connector %p \n", name_.c_str(), connector_.get());
    TcpConnectionPtr conn;
    bool unique = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        // 检查当前 shared_ptr 是否是唯一拥有该对象的指针
        unique = connection_.unique();
        conn = connection_;
    }
    // 如果连接存在
    if (conn)
    {
        CloseCallback cb = std::bind(&removeConnection, this, std::placeholders::_1);
        // 为什么要在IO线程中执行setCloseCallback?
        // 因为TcpConnection的生命周期是在IO线程中管理的、让TcpConnection自己管理自己的生命周期
        loop_->runInLoop(std::bind(&TcpConnection::setCloseCallback, conn, cb));
        if (unique) // 如果是唯一的shared_ptr、即只存在一个连接
        {
            conn->forceClose();
        }
    }
    else // 如果连接不存在
    {
        connector_->stop();
        // loop_->runAfter(1, std::bind(&removeConnctor, connector_));
    }
}

void TcpClient::connect()
{
    LOG_INFO("TcpClient::connect[%s] - connecting to %s \n", name_.c_str(), connector_->serverAddr().toIpPort().c_str());
    connect_ = true;
    connector_->start();
}

void TcpClient::disconnect()
{
    connect_ = false;
}

void TcpClient::stop()
{
    connect_ = false;
    connector_->stop();
}

void TcpClient::removeConnectionInLoop(EventLoop *loop, const TcpConnectionPtr &conn)
{
    loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

void TcpClient::removeConnection(const TcpConnectionPtr &conn)
{
    // 加锁、防止多线程下的竞争
    {
        std::lock_guard<std::mutex> lock(mutex_);
        connection_.reset();
    }
    // 在IO线程中执行
    loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    // 如果需要重连
    if (retry_ && connect_)
    {
        LOG_INFO("TcpClient::connect[%s] - Reconnecting to %s \n", name_.c_str(), connector_->serverAddr().toIpPort().c_str());
        // 重连
        connector_->restart();
    }
}

/**
 * @brief 新连接处理函数。Not thread safe, but in loop
 */
void TcpClient::newConnection(int sockfd)
{
    InetAddr peerAddr(Socket::getPeerAddr(sockfd));
    char buf[32];
    snprintf(buf, sizeof(buf), ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    InetAddr localAddr(Socket::getLocalAddr(sockfd));
    // 创建一个TcpConnection对象
    TcpConnectionPtr conn = std::make_shared<TcpConnection>(loop_, connName, sockfd, localAddr, peerAddr);
    // 设置连接回调函数
    conn->setConnectionCallback(connectionCallback_);
    // 设置消息回调函数
    conn->setMessageCallback(messageCallback_);
    // 设置写完成回调函数
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    // 设置关闭回调函数
    conn->setCloseCallback(std::bind(&TcpClient::removeConnection, this, std::placeholders::_1));
    // 加锁、防止多线程下的竞争
    {
        std::lock_guard<std::mutex> lock(mutex_);
        // 多个Connector会引起线程安全
        connection_ = conn;
    }
}