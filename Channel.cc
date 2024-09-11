#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
// 对读事件感兴趣
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
// 对写事件感兴趣
const int kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), // loop标记channel所属的loop
      fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
{
}

Channel::~Channel()
{
}

// 当一个TcpConnection新连接创建的时候调用
// channel一个弱智能指针指向一个TcpConnection对象，两者紧紧绑定
void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

// 当改变channel所表示的fd的event事件后，update负责在poller里面更改fd相应的事件
// EventLoop => ChannelList + Poller
void Channel::update()
{
    // 通过Channel所属的EventLoop，调用Poller的相应方法，注册fd的event事件
    loop_->updateChannel(this);
}

// 在Channel所属的EventLoop中把当前的Channel删除掉
void Channel::remove()
{
    loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
    if (tied_)
    {
        // 提升为强指针
        std::shared_ptr<void> guard = tie_.lock();
        if (guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

// 根据具体接收到的事件来执行相应的回调操作
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("CHANNEL HandelEvent revents:%d", revents_);
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        // 发生异常了
        if (closeCallback_)
        {
            closeCallback_();
        }
    }

    if (revents_ & EPOLLERR)
    {
        if (errorCallback_)
        {
            errorCallback_();
        }
    }

    if (revents_ & (EPOLLIN | EPOLLPRI))
    {
        if (readCallback_)
        {
            readCallback_(receiveTime);
        }
    }
    if (revents_ & EPOLLOUT)
    {
        if (writeCallback_)
        {
            writeCallback_();
        }
    }
}