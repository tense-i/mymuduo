#pragma once
#include <arpa/inet.h>
#include <netinet/in.h>

#include <string>

// 网络地址类--->封装了sockaddr_in结构体（ipv4地址结构体）
class InetAddr
{
private:
    sockaddr_in addr_;

public:
    explicit InetAddr(uint16_t port = 0, std::string ip = "127.0.0.1");
    explicit InetAddr(const sockaddr_in &addr);
    ~InetAddr();

    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;

    const sockaddr *getSockaddr() const;
    void setSockaddr(const sockaddr_in &addr);
};
