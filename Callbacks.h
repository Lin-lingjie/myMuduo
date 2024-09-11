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
// 当发送比接收快的多的时候
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr &, size_t)>;