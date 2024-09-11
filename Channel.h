#pragma once
#include <functional>
#include <memory>
#include "noncopyable.h"
#include "Timestamp.h"

// 只用到了类型的声明，没有用到类型具体的定义和方法，使用：前置声明(不能定义相应的对象，也不能访问它们的成员变量和成员函数)
// 用到的EventLoop为指针类型都是四个字节，可用前置声明
class EventLoop;

// 一个线程有一个event loop，一个event loop里面有一个polar， 一个polar可以监听很多channel
// channel 封装了sockfd和其感兴趣的event：如EPOLLIN、EPOLLOUT事件
// 还绑定了poller返回的具体事件
class Channel : noncopyable
{
public:
    // 事件回调
    using EventCallback = std::function<void()>;
    // 只读事件回调
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    // fd得到poller通知以后，处理事件的
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象
    // move()将参数转为右值，启用移动语义 ，避免深度拷贝
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // 防止当channel被手动remove掉，channel还在执行回调操作，弱智能指针来监听。
    void tie(const std::shared_ptr<void> &obj);

    int fd() const { return fd_; }
    int events() const { return events_; }
    int set_revents(int revt) { return revents_ = revt; }

    // 设置fd相应的事件状态
    void enableReading()
    {
        events_ |= kReadEvent;
        update();
    }
    void disableReading()
    {
        events_ &= ~kReadEvent;
        update();
    }
    void enableWriting()
    {
        events_ |= kWriteEvent;
        update();
    }
    void disableWriting()
    {
        events_ &= ~kWriteEvent;
        update();
    }
    void disableAll()
    {
        events_ |= kNoneEvent;
        update();
    }

    // 返回fd当前的事件状态
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    // channel属于那个loop
    EventLoop *ownerLoop() { return loop_; }
    // 删除channel
    void remove();

private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    // 没有对任何事件感兴趣
    static const int kNoneEvent;
    // 对读事件感兴趣
    static const int kReadEvent;
    // 对写事件感兴趣
    static const int kWriteEvent;

    EventLoop *loop_; // 事件循环
    const int fd_;    // fd,Poller监听的对象
    int events_;      // 注册fd感兴趣的事件,通过epoll_ctl注册
    int revents_;     // poller返回的具体发生的事件
    int index_;       // 用来标记channel在poller中的状态，与kNew/kAdded/kDeleted联合使用

    // 防止channel被remove掉的时候，还在使用channel
    std::weak_ptr<void> tie_;
    bool tied_;

    // poller给channel返回了最终发生的具体事件revents_,它负责调用具体事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};
