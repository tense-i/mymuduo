#include "TcpConnection.h"
#include "Socket.h"
#include "Channel.h"
#include "Logger.h"
#include "EventLoop.h"
#include "CallBacks.h"

#include <unistd.h>

/**
 * @brief 检查EventLoop是否为空
 */
static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d TcpConnection Loop is null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

/**
 * @param loop IO线程的EventLoop、即subReactor
 */
TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &nameArg,
                             int sockfd,
                             const InetAddr &localAddr,
                             const InetAddr &peerAddr)
    : loop_(CheckLoopNotNull(loop)),
      name_(nameArg),
      state_(kConnecting),
      reading_(true),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)), localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64 * 1024 * 1024)
{
    // 给channel设置相应的回调函数
    channel_->setReadCallBack(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallBack(
        std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallBack(
        std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallBack(
        std::bind(&TcpConnection::handleError, this));

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d \n",
             name_.c_str(), channel_->fd(), (int)state_);
}

/**
 * @brief 发送数据,保证每个fd的数据都是在自己的subloop线程中发送的
 */
void TcpConnection::send(const std::string &msg)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(msg.c_str(), msg.size());
        }
        else // 当前this所在的线程不是创建loop_的线程
        {
            // 再自己的线程中执行发送数据的操作
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, msg.c_str(), msg.size()));
        }
    }
}

/**
 * @brief 在自己的线程中关闭连接--写半关闭。

*/
void TcpConnection::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(
            std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

/**
 * @brief 连接建立

*/
void TcpConnection::connectEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading(); // 向poller注册channel的epollin事件

    // 新连接建立，执行回调
    connectionCallback_(shared_from_this());
}
/**
 * @brief 连接销毁
 */
void TcpConnection::connectDestroyed()
{
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll(); // 把channel的所有感兴趣的事件，从poller中del掉
        connectionCallback_(shared_from_this());
    }
    channel_->remove(); // 把channel从poller中删除掉
}

/**
 * @brief 读事件处理
 * @param receiveTime 事件发生的时间

*/
void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErr = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErr);
    if (n > 0)
    { // 已建立连接的用户，有可读事件发生了，调用用户传入的回调操作onMessage
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        errno = savedErr;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if (channel_->isWriting())
    {
        int savedErr = 0;
        // 将缓冲区数据写入到fd中
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErr);
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            // 如果outputBuffer_的可读内容为空，说明数据已经全部写完
            if (outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting(); // 不再关注epollout事件
                if (writeCompleteCallback_) //
                {                           // 唤醒loop_对应的thread线程，执行回调
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                } // 如果是关闭连接的状态，就调用shutdownInLoop
                if (state_ == kDisconnecting) // 区分kDisconnecting和kDisconnected
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else
    {
        LOG_ERROR("TcpConnection fd=%d is down, no more writing \n", channel_->fd());
    }
}

/**
 * @brief 关闭连接处理
 */
void TcpConnection::handleClose()
{
    LOG_INFO("TcpConnection::handleClose fd=%d state=%d \n", channel_->fd(), (int)state_);

    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());

    // 区分连接关闭的回调跟关闭连接的回调
    // 连接关闭---当连接关闭时，执行的回调
    // 关闭连接---手动的关闭连接
    connectionCallback_(connPtr); // 执行连接关闭的回调
    closeCallback_(connPtr);      // 关闭连接的回调  执行的是TcpServer::removeConnection回调方法
}

/**
 * @brief 错误处理

*/
void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    // 拿到错误号
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d \n", name_.c_str(), err);
}

/**
 * @brief 发送数据  应用写的快， 而内核发送数据慢， 需要把待发送数据写入缓冲区， 而且设置了水位回调
 */
void TcpConnection::sendInLoop(const void *data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultErr = false;
    // 已经断开连接了、之前调用过该connection的shutdown，不能再进行发送了
    if (state_ == kDisconnected)
    {
        LOG_ERROR("disconnected, give up writing!");
        return;
    }
    // 表示channel_第一次开始写数据，而且缓冲区没有待发送数据
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        // 直接调用write函数发送数据到fd
        nwrote = write(channel_->fd(), data, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote; // 剩余的数据
            if (remaining == 0 && writeCompleteCallback_)
            {
                // 既然在这里数据全部发送完成，就不用再给channel设置epollout事件了
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else
        {
            nwrote = 0;
            // EWoulDBLOCK是什么错误
            //  表示当前缓冲区已经满了，不能再写入数据了
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                // EPIPE表示对端已经关闭了连接、ECONNRESET表示连接被重置
                if (errno == EPIPE || errno == ECONNRESET) // SIGPIPE  RESET
                {
                    faultErr = true;
                }
            }
        }
    }

    // 说明数据没有一次性全部写完，剩余的数据需要保存到缓冲区当中，然后给channel
    // 设置epollout事件，当缓冲区有空间的时候，继续写数据
    if (!faultErr && remaining > 0)
    {

        size_t oldLen = outputBuffer_.readableBytes();
        // 如果缓冲区的数据超过了高水位标记，就调用高水位回调函数
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_)
        {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }

        outputBuffer_.append((char *)data + nwrote, remaining);
        if (!channel_->isWriting())
        {
            // 因为我们只是将数据写到了缓冲区而没有写到fd里面、当触发EPOLLOUT时就可以写入数据了
            channel_->enableWriting(); // 这里一定要注册channel的写事件，否则poller不会给channel通知epollout
        }
    }
}

void TcpConnection::shutdownInLoop()
{
    if (!channel_->isWriting()) // 说明outputBuffer中的数据已经全部发送完成
    {
        socket_->shutdownWrite(); // 关闭写端
    }
}
