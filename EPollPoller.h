#pragma once
#include "Poller.h"
#include "Timestamp.h"

#include <vector>
#include <sys/epoll.h>

class Channel;
/*
epoll的使用:
epoll_create
epoll_ctl add/mod/del
epoll_wait
*/

// poller里面直接操作的是channel，不是fd
class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;
    
    // epoll_wait
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;

    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;

private:
    static const int kInitEventListSize = 16;

    // 填写活跃的连接
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    // updateChannel与removeChannel都调用此函数，更新channel通道
    void update(int operation, Channel *channel);

    // epoll_event数组
    using EventList = std::vector<epoll_event>;
    int epollfd_;
    EventList events_;
};