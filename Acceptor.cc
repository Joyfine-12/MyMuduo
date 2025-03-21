#include "Acceptor.h"

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport) {

}
Acceptor::~Acceptor() {
    
}

void Acceptor::setNewConnectionCallback(const newConnectionCallback& cb) {
    newConnectionCallback_ = cb;
}

bool Acceptor::listenning() const { 
    return listenning_; 
}

void Acceptor::listen() {

}

void Acceptor::handleRead() {
    
} 