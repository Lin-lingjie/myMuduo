#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"
#include "Timestamp.h"

#include <errno.h>
#include <unistd.h>
#include <strings.h>

const int kNew = -1;    // 还没往epoll添加channel
const int kAdded = 1;   // channel已添加到epoll中
const int kDeleted = 2; // channel从epoll中删除

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop), epollfd_(::epoll_create1(EPOLL_CLOEXEC)), events_(kInitEventListSize)
{
    if (epollfd_ < 0)
    { 
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

void EPollPoller::updateChannel(Channel *channel)
{
    const int index = channel->index();
    LOG_INFO("func=%s fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(), channel->events(), index);
    // channel未添加过或者已删除，都可能会有ADD事件
    if (index == kNew || index == kDeleted)
    {
        if (index == kNew)
        {
            int fd = channel->fd();
            Channels_[fd] = channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else
    {
        // chnanel已经注册过了
        int fd = channel->fd();
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// 从poller中删除channel，但是还在channelList中
void EPollPoller::removeChannel(Channel *channel)
{
    int fd = channel->fd();
    Channels_.erase(fd);
    int index = channel->index();
    LOG_INFO("func=%s fd=%d  \n", __FUNCTION__, channel->fd());
    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

// 更新channel通道 EPOLL_CTL中的 add、mod、del
void EPollPoller::update(int operation, Channel *channel)
{
    epoll_event event;
    bzero(&event, sizeof event);
    int fd = channel->fd();
    event.events = channel->events();
    event.data.ptr = channel;
    event.data.fd = fd;

    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("EPOLL_CTL_DEL error:%d\n", errno);
        }
        else
        {
            LOG_FATAL("EPOLL_CTL_ADD/MOD error:%d\n", errno);
        }
    }
}

// epoll_wait
Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    // 实际上用LOG_DEBUG输出日志更为合理，免得频繁调用LOG_INFO影响效率
    LOG_INFO("func=%s fd total count=%lu\n", __FUNCTION__, Channels_.size());
     
    // &*events_. begin()取events_数组的起始 地址
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        LOG_INFO("%d events happended\n", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        // events_扩容
        if (numEvents == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        LOG_DEBUG("%s TIMEOUT! \n", __FUNCTION__);
    }
    else
    {
        // 外部中断也是要继续执行
        if (saveErrno != EINTR)
        {
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() err!");
        }
    }
    return now;
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for (int i = 0; i < numEvents; i++)
    {
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}