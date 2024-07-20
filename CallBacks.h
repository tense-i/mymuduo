#pragma once

#include <memory>
#include <functional>

class Buffer;
class TcpConnection;
class Timestamp;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
using CloseCallback = std::function<void(const TcpConnectionPtr &)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
using MessageCallback = std::function<void(const TcpConnectionPtr &,
                                           Buffer *,
                                           Timestamp)>;
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr &, size_t)>;

//
using TimerCallback = std::function<void()>; // 定时器回调函数

//
using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;    // 连接回调函数
using CloseCallback = std::function<void(const TcpConnectionPtr &)>;         // 关闭回调函数
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>; // 写完成回调函数

//
using removeConnectionCallback = std::function<void(EventLoop *loop, const TcpConnectionPtr &conn)>; // 移除连接回调函数
