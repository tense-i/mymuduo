#include "Connector.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"
#include "Socket.h"

#include <cstring> // Include the <cstring> header file
#include <unistd.h>

// const int Connector::kMaxRetryDelayMs;
Connector::Connector(EventLoop *loop, const InetAddr &serverAddr)
    : loop_(loop),
      serverAddr_(serverAddr),
      connect_(false),
      state_(Connector::KDisconnected),
      retryDelayMs_(Connector::KInitRetryDelayMs)
{
    LOG_DEBUG("ctor[%p]", this);
}

Connector::~Connector()
{
    LOG_DEBUG("dtor[%p]", this);
}

void Connector::start()
{
    connect_ = true;
    loop_->runInLoop(std::bind(&Connector::startInLoop, this)); //
}

void Connector::stop()
{
    connect_ = false;
    loop_->queueInLoop(std::bind(&Connector::stopInLoop, this));
}

void Connector::startInLoop()
{
    if (connect_)
    {
        connect();
    }
    else
    {
        LOG_DEBUG("do not connect");
    }
}

void Connector::stopInLoop()
{
    if (state_ == KConnecting)
    {
        setState(KDisconnected);
        int sockfd = removeAndResetChannel();
        retry(sockfd);
    }
}

/**
 * @brief 连接服务器
 */
void Connector::connect()
{
    int sockfd = Socket::createNonblockingFd();
    int ret = ::connect(sockfd, (sockaddr *)serverAddr_.getSockaddr(), sizeof(sockaddr_in)); // 连接服务器
    int savedErrno = (ret == 0) ? 0 : errno;                                                 // 0表示连接成功

    switch (savedErrno)
    {
    case 0:           // 连接成功
    case EINPROGRESS: // 连接正在进行中
    case EINTR:       // 被信号中断
    case EISCONN:     // 已经连接
        connecting(sockfd);
        break;
    case EAGAIN:        // 资源暂时不可用
    case EADDRINUSE:    // 地址已经被使用
    case EADDRNOTAVAIL: // 地址不可用
    case ECONNREFUSED:  // 连接被拒绝
    case ENETUNREACH:   // 网络不可达
        retry(sockfd);  // 重连
        break;
    default:
        LOG_ERROR("Connector::connect - %d \n", savedErrno);
        ::close(sockfd);
        break;
    }
}

/**
 * @brief 正在连接
 */
void Connector::connecting(int sockfd)
{
    setState(KConnecting);
    // 为什么使用reset，而不是直接赋值？
    // 因为channel_是一个unique_ptr，不能直接赋值。并且Connector是复用的，所以需要reset（具体看陈硕的LInux多线程服务端编程8.9）
    channel_.reset(new Channel(loop_, sockfd));
    channel_->setWriteCallBack(std::bind(&Connector::handleWrite, this)); // 设置写事件回调函数
    channel_->setErrorCallBack(std::bind(&Connector::handleError, this)); // 设置错误事件回调函数
    channel_->enableWriting();                                            // 开启写事件监听
}
void Connector::resetChannel()
{
    channel_.reset(); // 重置channel_
}

/**
 * @brief 在指定的时间后重连

*/
void Connector::retry(int sockfd)
{
    ::close(sockfd); //
    setState(KDisconnected);
    if (connect_)
    {
        LOG_INFO("Connector::retry - Retry connecting to %s in %d milliseconds. \n", serverAddr_.toIpPort().c_str(), retryDelayMs_);
        // retryDelayMs_ / 1000.0 毫秒转换为秒
        loop_->runAfter(retryDelayMs_ / 1000.0, std::bind(&Connector::startInLoop, shared_from_this())); // 在retryDelayMs_/1000 时间后重连
        retryDelayMs_ = std::min(retryDelayMs_ * 2, Connector::KMaxRetryDelayMs);                        // 重连时间翻倍
    }
    else
    {
        LOG_DEBUG("do not connect");
    }
}
int Connector::removeAndResetChannel()
{
    channel_->disableAll();
    channel_->remove();

    int sockfd = channel_->fd();
    // 在Channel::handleEvent中不能重置channel_，因为我们在Channel::handleEvent中
    // 还要使用channel_，所以这里不能reset
    loop_->queueInLoop(std::bind(&Connector::resetChannel, this)); // 重置channel_
    return sockfd;
}

/**
 * @brief 处理Connector的写事件
 */
void Connector::handleWrite()
{
    LOG_INFO("Connector::handleWrite state:%d \n", state_);

    // 正在连接
    if (state_ == KConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = Socket::getSocketError(sockfd);

        if (err)
        {
            LOG_ERROR("Connector::handleWrite");
            retry(sockfd); //   重连
        }
        else if (Socket::isSelfConnect(sockfd))
        {
            LOG_ERROR("Connector::handleWrite - Self connect \n");
            retry(sockfd); // 重连
        }
        else
        {
            setState(KConnected);
            if (connect_)
            {
                newConnectionCallback_(sockfd); // 连接成功回调
            }
            else
            {
                ::close(sockfd);
            }
        }
    }
    else
    {
        LOG_ERROR("Connector::handleWrite - state:%d \n", state_);
    }
}

void Connector::handleError()
{
    LOG_ERROR("Connector::handleError state:%d \n", state_);
    if (state_ == KConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = Socket::getSocketError(sockfd);
        LOG_ERROR("SO_ERROR:%d \n", err);
        retry(sockfd);
    }
}
