#pragma once

#include "Poller.h"
#include "Timestamp.h"

#include <vector>
#include <sys/epoll.h>

/*
epoll的使用
epoll_create
epoll_ctl add/modify/del
epoll_wait
*/
class EpollPoller : public Poller {
public:
    EpollPoller(EventLoop* loop);
    ~EpollPoller() override; // 覆盖纯虚函数

    // 重写基类Poller的抽象方法
    Timestamp poll(int timeoutMs, ChannelList* activeChannelLs) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override; 
private:
    static const int kInitEventListSize = 16; // 初始EventList = std::vector<epoll_event>长度

    // 填写活跃的连接
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    // 更新Channel通道
    void update(int operation, Channel* channel);

    using EventList = std::vector<epoll_event>;

    int epollfd_;
    EventList events_;
};