#include "InetAddr.h"
#include <string.h>

InetAddr::InetAddr(uint16_t port, std::string ip)
{
    bzero(&addr_, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
}

InetAddr::InetAddr(const sockaddr_in &addr)
{
    addr_.sin_addr = addr.sin_addr;
    addr_.sin_family = addr.sin_family;
    addr_.sin_port = addr.sin_port;
}

InetAddr::~InetAddr()
{
}

std::string InetAddr::toIp() const
{
    char buf[64] = "";
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    return buf;
}

std::string InetAddr::toIpPort() const
{
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    size_t end = strlen(buf);
    uint16_t port = ntohs(addr_.sin_port);
    sprintf(buf + end, ":%u", port);
    return buf;
}

uint16_t InetAddr::toPort() const
{
    return addr_.sin_port;
}

const sockaddr *InetAddr::getSockaddr() const
{
    return (sockaddr *)&addr_;
}
