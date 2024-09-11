#include "InetAddress.h"

#include <strings.h>
#include <string.h>

// port端口号、ip为ip地址
InetAddress::InetAddress(uint16_t port, std::string ip)
{
    bzero(&addr_, sizeof addr_);
    // 设置地址族
    addr_.sin_family = AF_INET;
    // 设置端口号
    addr_.sin_port = htons(port);
    // 设置IP地址
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
}

// 输出IP地址，从addr_读出IP地址， 前面已经转换成网络字节序了，我们要把它转为本地字节序
std::string InetAddress::toIp() const
{
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    return buf;
}

// 输出端口号
uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
}

// Ip+Port返回回去
std::string InetAddress::toIpPort() const
{
    // toIp
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    size_t end = strlen(buf);
    // toPort
    uint16_t port = ntohs(addr_.sin_port);
    sprintf(buf + end, ":%u", port);
    return buf;
}

