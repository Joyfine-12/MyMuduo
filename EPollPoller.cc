#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <errno.h>
#include <strings.h>
#include <unistd.h>

// channel 未添加到poller中
const int kNew = -1; // cahnnel 的成员index_ = -1
// channel 已添加到poller中
const int kAdded = 1;
// channel 从poller中删除
const int kDeleted = 2;

EpollPoller::EpollPoller(EventLoop* loop) 
    : Poller(loop) 
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(kInitEventListSize) { // vector<epoll_event>
    if (epollfd_ < 0) {
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}

EpollPoller::~EpollPoller() {
    ::close(epollfd_);
}

Timestamp EpollPoller::poll(int timeoutMs, ChannelList* activeChannelLs) {
    // 实际上应该用LOG_DEBUG输出更加合理
    LOG_INFO("func = %s => fd tatal count: %lu\n", __FUNCTION__, channels_.size());

    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());
    
    if (numEvents > 0) {
        LOG_INFO("%d events happened \n", numEvents);
        fillActiveChannels(numEvents, activeChannelLs);

        if (numEvents == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    } else if (numEvents == 0){
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    } else {
        if (saveErrno != EINTR) {
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() err");
        }
    }
    return now;
}

// channel update remove => EventLoop updateChannel removeChannel => Poller updateChannel removeChannel
/*
             EventLoop => poller.poll
    ChannnelList     Poller
                ChannelMap<fd, channel*>
*/
void EpollPoller::updateChannel(Channel* channel) {
    const int index = channel -> index();
    LOG_INFO("func = %s => fd = %d events = %d index = %d \n", __FUNCTION__, channel -> fd(), channel -> events(), index);

    if (index == kNew || index == kDeleted) {
        if (index == kNew) {
            int fd = channel -> fd();
            channels_[fd] = channel;
        }

        channel -> set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    } else {
        int fd = channel -> fd();
        if (channel -> isNoneEvent()) {
            channel -> set_index(kDeleted);
            update(EPOLL_CTL_DEL, channel);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// 从poller中删除channel的逻辑
void EpollPoller::removeChannel(Channel* channel) {
    int fd = channel -> fd();
    channels_.erase(fd);
    
    LOG_INFO("func = %s => fd = %d \n", __FUNCTION__, fd);

    int index = channel -> index();
    if(index == kAdded) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel -> set_index(kNew);
}

// 填写活跃连接
void EpollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const {
    for (int i = 0; i < numEvents; ++i) {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel -> set_revents(events_[i].events);
        activeChannels -> push_back(channel); // EventLoop 就拿到他的poller给它返回的所有发生事件的channel列表了
    }
}

// 更新Channel通道 epoll_ctl add/modify/del
void EpollPoller::update(int operation, Channel* channel) {
    epoll_event event;
    bzero(&event, sizeof event);
    
    int fd = channel -> fd();

    event.events = channel -> events(); //返回fd所有感兴趣的事件
    event.data.fd = fd;
    event.data.ptr = channel;

    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
        if (operation == EPOLL_CTL_DEL) {
            LOG_ERROR("epoll_ctl del error: %d\n", errno);
        } else {
            LOG_FATAL("epoll_ctl add/mod fatal: %d\n", errno);
        }
    }
}
