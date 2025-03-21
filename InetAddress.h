#pragma once

#include <arpa/inet.h> // 网络编程 中与 Internet 地址操作
#include <netinet/in.h> //网络编程 中与 Internet 协议（IP） 相关的数据结构和常量的定义
#include <string>

// 封装socket地址类型
class InetAddress {
public:
    explicit InetAddress(uint16_t port, std::string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in &addr);
    
    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const; //获取port端口号

    //获取成员变量
    const sockaddr_in* getSockAddr() const;
    void setSockAddr(const sockaddr_in &addr);
private:
    // 用于表示 IPv4 地址和端口号的结构体。
    sockaddr_in addr_;
};