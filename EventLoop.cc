#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

// 防止一个线程创建多个EventLoop thread_local
__thread EventLoop* t_loopInThisThread = nullptr;

// 定义默认的Poller IO复用的接口的超时时间
const int kPollTimeMs = 10000;

// 创建wakeupfd， 用来notify唤醒subReactor处理新来的channel 
int CreateEventfd() {
    // 用来唤醒的
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) {
        LOG_FATAL("eventfd error:%d \n", errno);
    } 
    return evtfd;
}

EventLoop::EventLoop() 
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(CurrentThread::tid())
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(CreateEventfd())
    , wakeupChannel_(new Channel(this, wakeupFd_)) {
        LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
        if (t_loopInThisThread) {
            LOG_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
        } else {
            t_loopInThisThread = this;
        }

        // 设置wakeupfd的事件类型以及发生事件后的回调操作
        wakeupChannel_ -> setReadCallback(std::bind(&EventLoop::handleRead, this));
        // 每一个EventLoop都将监听wakeupChannel的EPOLLIN读事件
        wakeupChannel_ -> enableReading();
    }
EventLoop::~EventLoop() {
    wakeupChannel_ -> disableAll();
    wakeupChannel_ -> remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}
// 开启事件循环
void EventLoop::loop() {
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start Looping \n", this);

    while(!quit_) {
        activeChannels_.clear();
        // 主要是监听两类fd  一种是client的fd， 一种是wakeupfd
        pollReturnTime_ = poller_ -> poll(kPollTimeMs, &activeChannels_);
        for (Channel* channel : activeChannels_) {
            // Poller 监听哪些channel发生事件了，然后上报给EventLoop，通知channel处理相应的事件
            channel -> handleEvent(pollReturnTime_);
        }

        // 执行当前EventLoop事件循环需要处理的回调操作
        /*
        IO 线程 mainloop accept fd <= channel subloop
        mainloop 事先注册一个回调cb(需要subloop来执行)  wakeup subloop之后 执行下面的方法 执行之前mianloop注册的cb操作
        */
        doPendingFunctiors();
    }

    LOG_INFO("EventLoop %p stop looping. \n", this);
    looping_ = false;
}

// 退出事件循环 1.loop 在自己的线程之中调用quit 2.在非loop的线程中调用quit
void EventLoop::quit() {
    quit_ = true;

    if (!isInLoopThread()) { //如果在其他线程中 调用quit 比如在一个subloop（woker）中调用了mainLoop(IO)的quit
        wakeup();
    }
}

Timestamp EventLoop::pollReturnTime() const { 
    return pollReturnTime_; 
}

// 在当前loop中执行cb
void EventLoop::runInLoop(Functor cb) {
    // 在当前的loop线程中 执行cb
    if (isInLoopThread()) { 
        cb();
    } else { // 在非当前loop中执行cb 就要唤醒loop所在线程执行cb
        queueInLoop(cb);
    }
}
// 把cb放进队列中，唤醒loop所在的线程，执行cb
void EventLoop::queueInLoop(Functor cb) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    // 唤醒相应的，需要执行上面回调操作的loop线程了
    // || callingPendingFunctors_的意思是：当前loop正在执行回调，但是loop又有了新的回调
    if (!isInLoopThread() || callingPendingFunctors_) { 
        wakeup(); // 唤醒loop所在的线程
    }
}

// 唤醒loop所在的线程 向wakeupfd_ 写一个数据 wakeupChannel就发生读事件，当前loop线程就会被唤醒
void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERROR("EventLoop::wakeup() write %lu bytes intead of 8 \n", n);
    }
}

// EventLoop的方法 => Poller的方法
void EventLoop::updateChannel(Channel *channel) {
    poller_ -> updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel) {
    poller_ -> removeChannel(channel);
}
bool EventLoop::hasChannel(Channel *channel) {
    return poller_ -> hasChannel(channel);
}

bool EventLoop::isInLoopThread() const { 
    return threadId_ == CurrentThread::tid(); 
}

//执行回调
void EventLoop::doPendingFunctiors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (const Functor &functors : functors) {
        functors(); // 执行当前loop需要执行的回调操作
    }

    callingPendingFunctors_ = false;
}

void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERROR("EventLoop::handleRead() reads %ld bytes instead of 8", n);
    }
}