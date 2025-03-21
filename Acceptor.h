#pragma once

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"
// #include "InetAddress.h"

#include <functional>

class EventLoop;
class InetAddress;

class Acceptor
{
public:
    using newConnectionCallback = std::function<void(int sockfd, const InetAddress&)>;

    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const newConnectionCallback& cb);

    bool listenning() const;
    void listen();
private:
    void handleRead();    

    EventLoop* loop_; // Acceptor用的就是用户定义的那个baseloop，也称作mainLoop
    Socket acceptSocket_;
    Channel acceptChannel_;
    newConnectionCallback newConnectionCallback_;
    bool listenning_;
};