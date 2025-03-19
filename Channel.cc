#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

// EventLoop: ChannelList Poller
Channel::Channel(EventLoop *loop, int fd) 
    : loop_(loop)
    , fd_(fd)
    , events_(0)
    , revents_(0)
    , index_(-1)
    , tied_(false) 
    {}     

Channel::~Channel() {}

void Channel::setReadCallback(ReadEventCallback cb) {
    readCallback_ = std::move(cb); //还不是很懂
}
void Channel::setWriteCallback(EventCallback cb) {
    writeCallback_ = std::move(cb); 
} 
void Channel::setCloseCallback(EventCallback cb) { 
    closeCallback_ = std::move(cb); 
}
void Channel::setErrorCallback(EventCallback cb) { 
    errorCallback_ = std::move(cb); 
}

//在其他什么地方被设置过？
void Channel::tie(const std::shared_ptr<void>& obj) { //weak 绑定 shared
    tie_ = obj;
    tied_ = true;
}

int Channel::fd() const {
    return fd_; 
}
int Channel::events() const { 
    return events_; 
}
int Channel::set_revents(int revt) { 
    revents_ = revt; 
}
/*
当改变channel所表示的events事件后，update负责在poller里面更改fd相应的事件epoll_ctl
EventLoop => ChannelList Poller
*/
void Channel::update() {
    // 通过channel 所属的EventLoop，调用poller的相应方法，注册fd的events事件
    // add code...
    // loop_ -> updateChannel(this);
}

// 在channel所属的eventloop中删除该channel删除
void Channel::remove() {
    //add code...
    // loop_ -> removeChannel(this);
}

// fd 得到poller通知以后，处理事件的
void Channel::handleEvent(Timestamp receiveTime) {
    if (tied_) {
        std::shared_ptr<void> guard = tie_.lock();
        if (guard) {
            handleEventWithGuard(receiveTime);
        }
    } else {
        handleEventWithGuard(receiveTime);
    }
}

// 根据具体接收到的事件 执行相应的回调操作即可
// 根据poller通知的channel的具体事件，由channel负责调用具体的回调函数
void Channel::handleEventWithGuard(Timestamp receiveTime) {
    LOG_INFO("channel handleEvent revents:%d", revents_);

    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) { // EPOLLHUP 表示文件描述符（如 socket）上发生了挂起事件
        if (closeCallback_) {
            closeCallback_();
        }
    }

    if (revents_ & EPOLLERR) {
        if (errorCallback_) {
            errorCallback_();
        }
    }

    if (revents_ & (EPOLLIN | EPOLLPRI)) {
        if (readCallback_) {
            readCallback_(receiveTime);
        }
    }

    if (revents_ & EPOLLOUT) {
        if (writeCallback_) {
            writeCallback_();
        }
    }
}

// update 调用epoll_ctl 即重新设置epoll监听结构
void Channel::enableReading() { 
    events_ |= kReadEvent; update(); 
} 
void Channel::disableReading() {
    events_ &= ~kReadEvent; update(); 
}
void Channel::enableWriting() { 
    events_ |= kWriteEvent; update(); 
}
void Channel::disableWriting() { 
    events_ &= kWriteEvent; update(); 
}
void Channel::disableAll() { 
    events_ = kNoneEvent; update(); 
}

// 当前fd有没有注册感兴趣的事件
bool Channel::isNoneEvent() const { 
    return events_ == kNoneEvent; 
} 
bool Channel::isWriting() const { 
    return events_ & kWriteEvent; 
}
bool Channel::isReading() const { 
    return events_ & kReadEvent; 
}


int Channel::index() { 
    return index_; 
}
void Channel::set_index(int idx) { 
    index_ = idx; 
}

// one loop per thread 一个线程一个eventlop一个poller监听多个channel
EventLoop* Channel::ownerLoop() { 
    return loop_; 
} //当前channel属于哪个eventloop