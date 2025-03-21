#include "Poller.h"
#include "EPollPoller.h"

#include <stdlib.h>

Poller* Poller::newDefaultPoller(EventLoop* loop) {
    if (::getenv("MUDUO_USE_POLL")) { // 获取环境变量
        return nullptr; // 生成poll的实例
    } else {
        return new EpollPoller(loop); // 生成epoll的实例
    }
}