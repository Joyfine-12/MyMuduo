#include <strings.h>
#include <string.h>

#include "InetAddress.h"

InetAddress::InetAddress(uint16_t port, std::string ip) {
    bzero(&addr_, sizeof addr_);
    addr_.sin_family = AF_INET; //设置协议族为Ipv4协议族
    addr_.sin_port = htons(port); //将主机字节序 转换为网络字节序
    addr_.sin_addr.s_addr = inet_addr(ip.c_str()); //点分十进制字符串（如 "192.168.1.1"）转换为 32 位无符号整数（网络字节序）
    //  ip.c_str() std::string 转换为 C 风格字符串 许多 C 标准库函数（如 inet_addr、inet_pton 等）和系统调用（如 socket 相关函数）需要 C 风格字符串 作为参数。
}

InetAddress::InetAddress(const sockaddr_in &addr)
    : addr_(addr)
    {}

std::string InetAddress::toIp() const {
    // addr_ 
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf); //二进制网络字节序 的 IPv4 地址转换为 点分十进制字符串 
    return buf;
}

std::string InetAddress::toIpPort() const {
    // ip:port
    //处理ip
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    //处理port
    size_t end = strlen(buf); // size_t 表示非负值
    uint16_t port = ntohs(addr_.sin_port); // 将网络字节序转化位主机字节序
    sprintf(buf + end, ":%u", port); //以 :端口号 的格式追加到字符数组 buf 的指定位置
    return buf;
}

// 获取port端口号
uint16_t InetAddress::toPort() const {
    // port
    return ntohs(addr_.sin_port);
}

//获取成员变量
const sockaddr_in* InetAddress::getSockAddr() const {
    return &addr_;
}


// #include <iostream>
// int main() {
//     InetAddress addr(8080);
//     std::cout << addr.toIpPort() << "\n";
//     std::cout << addr.toIp() << "\n";
//     std::cout << addr.toPort() << "\n";

//     return 0;
// }