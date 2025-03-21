#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

class Channel;
class Poller;

//时间循环类  主要包含两个大模块 Channel  Poller(epoll的抽象) 相当于reactor反应堆
class EventLoop : noncopyable {
public:
    // 事件回调
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void loop();
    // 退出事件循环
    void quit();

    Timestamp pollReturnTime() const;
    
    // 在当前loop中执行cb
    void runInLoop(Functor cb);
    // 把cb放进队列中，唤醒loop所在的线程，执行cb
    void queueInLoop(Functor cb);

    // 用来唤醒loop所在的线程的
    void wakeup();

    // EventLoop的方法 => Poller的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // 判断EventLopp对象是否在自己的线程里面
    bool isInLoopThread() const;
private:
    void handleRead(); // wake up
    void doPendingFunctiors(); //执行回调

    using ChannelList = std::vector<Channel*>;
    
    std::atomic_bool looping_; //原子操作的bool值， 通过CAS实现的
    std::atomic_bool quit_; // 标志退出loop循环

    const pid_t threadId_; //记录当前loop所在线程的id

    Timestamp pollReturnTime_; // poller返回发生事件的Channels的时间点
    std::unique_ptr<Poller> poller_;

    int wakeupFd_; // 当mainLoop获取一个新用户的channel，通过轮询算法选择一个subloop，通过该成员唤醒sub_loop处理channel
    std::unique_ptr<Channel> wakeupChannel_;
    
    ChannelList activeChannels_;

    std::atomic_bool callingPendingFunctors_; // 标识当前loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_; // 存储loop所有需要的回调操作
    std::mutex mutex_; // 互斥锁，用来保护上面的容器线程安全操作
};
