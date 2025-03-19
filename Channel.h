#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory> // using smart pointer 

// 前向声明
class EventLoop;;

/*
理清楚 EventLoop、Channel、Poller之间的关系 在Reactor模型上对应  Demultiplex 多路事件分发器
Channel 理解位通道，封装了sockfd和其刚兴趣的event，如EPOLLIN、EPOLLOUT事件
还绑定了 poller 返回的具体事件 
根据revents 根据当前通道里面的fd返回 知道当前事件 然后回调函数
*/
class Channel : noncopyable{
public:
    using EventCallback = std::function<void()>; //事件回调函数 回调函数：在事件发生时被调用的函数，用于处理事件
    using ReadEventCallback = std::function<void(Timestamp)>; //只读事件回调

    Channel(EventLoop *loop, int fd);
    ~Channel();

    // fd得到poller通知以后，处理时间的
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb);
    void setWriteCallback(EventCallback cb); 
    void setCloseCallback(EventCallback cb);
    void setErrorCallback(EventCallback cb);

    // 防止channel被手动remove掉，channel还在执行回调操作
    void tie(const std::shared_ptr<void>&); //只可以在.h  文件中这么写 不显示参数名

    int fd() const;
    int events() const;
    int set_revents(int revt); // epoll 监听事件之后设置 revent 真真正正发生的事件

    // 设置fd相应的事件状态
    void enableReading(); // update 调用epoll_ctl 即重新设置epoll监听结构
    void disableReading();
    void enableWriting(); 
    void disableWriting();
    void disableAll();

    // 返回fd当前的感兴趣事件状态
    bool isNoneEvent() const; // 当前fd有没有注册感兴趣的事件
    bool isWriting() const;
    bool isReading() const;

    int index();
    void set_index(int idx);
    
    // one loop per thread 一个线程一个eventlop一个poller监听多个channel
    EventLoop* ownerLoop(); //当前channel属于哪个eventloop
    void remove(); // 删除channel用的

private:

    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    //表示当前事件的状态
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_; // 事件循环
    const int fd_; // fd, poller 监听的socket对象
    int events_; // 注册fd 感兴趣的事件
    int revents_; // poller 返回的具体发生的事件
    int index_; // used by poller

    std::weak_ptr<void> tie_;
    bool tied_;

    // 因为channel通道里面能够获知fd最终发生的具体事件revents，所以它负责调用具体事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};