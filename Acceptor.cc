#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
// #include <netinet/in.h> // 包含 IPPROTO_TCP 的定义
#include <unistd.h>

static int createNonblocking() {
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sockfd < 0) {
        LOG_FATAL("%s:%s:%d listen socket create err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
    } else {
        return sockfd;
    }
}

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport) 
    : loop_(loop)
    , acceptSocket_(createNonblocking())
    , acceptChannel_(loop, acceptSocket_.fd())
    , listenning_(false) {
        acceptSocket_.setReuseAddr(true);
        acceptSocket_.setReusePort(true);
        acceptSocket_.bindAddress(listenAddr);  // bind
        // TcpServer::Start() Acceptor.listen 如果有新用户连接 ，要执行一个回调 connfd => channel => subloop
        // baseLoop => acceptChannel_(listenfd) => 
        acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
    }

Acceptor::~Acceptor() {
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::setNewConnectionCallback(const newConnectionCallback& cb) {
    newConnectionCallback_ = cb;
}

bool Acceptor::listenning() const { 
    return listenning_; 
}

void Acceptor::listen() {
    listenning_ = true;
    acceptSocket_.listen(); // listen
    acceptChannel_.enableReading(); // acceptChannel_ => Poller
}

// listenfd 有事件发生了，就是有新用户连接了
void Acceptor::handleRead() {
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0) {
        if (newConnectionCallback_) {
            newConnectionCallback_(connfd, peerAddr); // 轮询找到subloop 唤醒 分发当前新客户端的Channel
        } else {
            ::close(connfd);
        }
    } else {
        LOG_FATAL("%s:%s:%d accept err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
        if (errno == EMFILE) {
            LOG_FATAL("%s:%s:%d sockfd reached limit! \n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
} 