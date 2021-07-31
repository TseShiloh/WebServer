/*
4. InetAddress.h/InetAddress.cc（InetAddress类）
- 网际地址sockaddr_in封装
*/
#ifndef MUDUO_NET_INETADDRESS_H
#define MUDUO_NET_INETADDRESS_H

#include <WebServer/base/copyable.h>
#include <WebServer/base/StringPiece.h>

#include <netinet/in.h>

namespace muduo
{
    namespace net
    {
        class InetAddress : public muduo::copyable
        {
        private:
            struct sockaddr_in addr_;
        public:
            /// Constructs an endpoint with given port number.
            /// Mostly used in TcpServer listening.
            // 仅仅指定port，不指定ip，则ip为INADDR_ANY（即0.0.0.0）
            explicit InetAddress(uint16_t port);

            /// Constructs an endpoint with given ip and port.
            /// @param ip should be "1.2.3.4"
            InetAddress(const StringPiece& ip, uint16_t port);

            /// Constructs an endpoint with given struct @param sockaddr_in
            /// Mostly used when accepting new connections
            InetAddress(const struct sockaddr_in& addr)
                : addr_(addr)
            {}

            string toIp() const;
            string toIpPort() const;

            // __attribute__ ((deprecated)) 表示该函数是过时的，被淘汰的
            // 这样使用该函数，在编译的时候，会发出警告
            string toHostPort() const __attribute__ ((deprecated))
            { return toIpPort(); }

            // default copy/assignment are Okay

            const struct sockaddr_in& getSockAddrInet() const { return addr_; }// 直接返回
            void setSockAddrInet(const struct sockaddr_in& addr) { addr_ = addr; }// 直接设置

            uint32_t ipNetEndian() const { return addr_.sin_addr.s_addr; }  // 返回网络字节序的IP（32位）
            uint16_t portNetEndian() const { return addr_.sin_port; }       // 返回网络字节序的端口（16位）

        };// class InetAddress
        
    } // namespace net
    
} // namespace muduo

#endif // MUDUO_NET_INETADDRESS_H
