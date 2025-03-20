#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map> // 无需对键值排序 所以用哈希表更快

class Channel;
class EventLoop;

// muduo 库中多路事件分发器的核心 IO服用模块 负责事件监听
class Poller : noncopyable {
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop* loop);
    virtual ~Poller();

    // 给所有的IO复用保留统一的接口 = 0表示没有默认实现 且要求派生类强制实现 是个纯虚函数
    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0; // 超时时间 激活的Channel 启动的
    virtual void updateChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;

    // 判断参数channel是否在当前Poller当中
    bool hasChannel(Channel* channel) const;

    // EventLoop可以通过该接口获取默认的IO复用的具体实现 所以不能再Poller里面实现 因为需要派生类产生具体实例 且Poller.cc 不能调用派生类
    // static 静态成员函数 属于类本身 可以直接通过类名调用
    static Poller* newDefaultPoller(EventLoop* loop);
protected:
    // int 表示map的key： sockfd value：sockfd所属的channel通道类型
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;
private:
    EventLoop* ownerLoop_; // 定义Pooler 所属的事件循环
};