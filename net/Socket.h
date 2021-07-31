/*
3. Socket.h/Socket.cc（Socket类）
- 用RAII方法封装socket file descriptor
*/
#ifndef MUDUO_NET_SOCKET_H
#define MUDUO_NET_SOCKET_H

#include <boost/noncopyable.hpp>

namespace muduo
{
    ///
    /// TCP networking
    ///
    namespace net
    {
        class InetAddress;

        ///
        /// Wrapper of socket file descriptor.
        ///
        /// It closes the sockfd when desctructs.
        /// It's thread safe, all operations are delegated to OS. 
        class Socket : boost::noncopyable
        {
        private:
            const int sockfd_;

        public:
            explicit Socket(int sockfd)
                : sockfd_(sockfd)
            {}

            // Socket(Socket&&) // move constructor in C++11
            ~Socket();

            int  fd() const { return sockfd_; }// 返回文件描述符

            /// abort if address in use
            void bindAddress(const InetAddress& localaddr);
            void listen();

            /* 
            - On success, returns a non-negative integer that is
            a descriptor for the accepted socket, which has been
            set to non-blocking and close-on-exec. *peeraddr is assigned.
            - On error, -1 is returned, and *peeraddr is untouched.
            */
            int  accept();
            void shutdownWrite();

            ///
            /// Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
            ///
            // Nagle算法可以一定程度上避免网络拥塞（延迟等待、合并发送，一般来说是200ms）
            // TCP_NODELAY选项可以禁用Nagle算法
            // 禁用Nagle算法，可以避免连续发包出现延迟，这对于编写低延迟的网络服务很重要
            void setTcpNoDelay(bool on);

            ///
            /// Enable/disable SO_REUSEADDR
            /// 
            // 设置地址重复利用
            void setReuseAddr(bool on);

            ///
            /// Enable/disable SO_KEEPALIVE
            ///
            // TCP keepalive是指定期探测连接是否存在，如果应用层有心跳的话，这个选项不是必需要设置的
            void setKeepAlive(bool on);
        }; // class Socket
    } // namespace net
    
} // namespace muduo

names

#endif // MUDUO_NET_SOCKET_H