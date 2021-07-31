/*
4. InetAddress.h/InetAddress.cc（InetAddress类）
- 网际地址sockaddr_in封装
*/
#include <WebServer/net/InetAddress.h>

#include <WebServer/net/Endian.h>
#include <WebServer/net/SocketsOps.h>

#include <strings.h>  // bzero
#include <netinet/in.h>

#include <boost/static_assert.hpp>

// INADDR_ANY use (type)value casting.
#pragma GCC diagnostic ignored "-Wold-style-cast"
static const in_addr_t kInaddrAny = INADDR_ANY;
#pragma GCC diagnostic error "-Wold-style-cast"

using namespace muduo;
using namespace muduo::net;

// 断言InetAddress类大小==sockaddr_in，因为该类只有一个数据成员addr_
BOOST_STATIC_ASSERT(sizeof(InetAddress) == sizeof(struct sockaddr_in));

InetAddress::InetAddress(uint16_t port)
{
    bzero(&addr_, sizeof addr_);
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = sockets::hostToNetwork32(kInaddrAny);
    addr_.sin_port = sockets::hostToNetwork16(port);
}

InetAddress::InetAddress(const StringPiece& ip, uint16_t port)
{
    bzero(&addr_, sizeof addr_);
    sockets::fromIpPort(ip.data(), port, &addr_);
}

string InetAddress::toIpPort() const
{
    char buf[32];
    sockets::toIpPort(buf, sizeof buf, addr_);
    return buf;
}

string InetAddress::toIp() const
{
    char buf[32];
    sockets::toIp(buf, sizeof buf, addr_);
    return buf;
}