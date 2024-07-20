#include "InetAddr.h"
#include "Channel.h"
#include "noncopyable.h"

class EventLoop;

class Connector : noncopyable, public std::enable_shared_from_this<Connector>
{
public:
    Connector(EventLoop *loop, const InetAddr &serverAddr);
    ~Connector();
    typedef std::function<void(int sockfd)> NewConnectionCallback;
    void setNewConnectionCallback(const NewConnectionCallback &cb)
    {
        newConnectionCallback_ = cb;
    }

    void start();   // can be called in any thread // 可以在任意线程调用
    void restart(); // must be called in loop thread // 必须在loop所在的线程中调用
    void stop();    // can be called in any thread // 可以在任意线程调用
private:
    enum States
    {
        KDisconnected, // 未连接
        KConnecting,   // 正在连接
        KConnected     // 已连接
    };

    static const int KMaxRetryDelayMs = 30 * 1000; // 30s
    static const int KInitRetryDelayMs = 500;      // 0.5s

    EventLoop *loop_;     // 连接所属的EventLoop
    InetAddr serverAddr_; // 服务器的地址
    bool connect_;        // 是否连接

    std::unique_ptr<Channel> channel_;            // 连接所对应的channel
    States state_;                                // 连接的状态
    int retryDelayMs_;                            // 重连的延迟时间、单位为ms
    NewConnectionCallback newConnectionCallback_; // 连接建立时的回调函数

private:
    void startInLoop();          // 只能在loop所在线程中调用
    void connect();              // 连接服务器
    void connecting(int sockfd); // 连接成功
    void setState(States s) { state_ = s; }
    void handleWrite();          // 写事件处理
    void handleError();          // 错误处理
    int removeAndResetChannel(); // 移除并重新设置channel的fd
    void resetChannel();         // 重新设置channel的fd
    void retry(int sockfd);      // 重连
};
