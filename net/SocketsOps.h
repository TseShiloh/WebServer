/*
2.SocketsOps.h/ SocketsOps.cc
- 封装了socket相关系统调用（全局函数，位于muduo::net::sockets名称空间中）.
*/
#ifndef MUDUO_NET_SOCKETSOPS_H
#define MUDUO_NET_SOCKETSOPS_H

#include <arpa/inet.h>

namespace muduo
{
    namespace net
    {
        namespace sockets
        {
            ///
            /// Creates a non-blocking socket file descriptor, abort if any error.
            /// 创建非阻塞的套接字，创建失败就中止
            int createNonblockingOrDie();

            int     connect(int sockfd, const struct sockaddr_in& addr);
            void    bindOrDie(int sockfd, const struct sockaddr_in& addr);
            void    listenOrDie(int sockfd);
            int     accept(int sockfd, struct sockaddr_in* addr);
            ssize_t read(int sockfd, void*buf, size_t count);
            ssize_t readv(int sockfd, const struct iovec *iov, int iovcnt);
            ssize_t write(int sockfd, const void *buf, size_t count);
            void    close(int sockfd);
            void    shutdownWrite(int sockfd);

            void toIpPort(char* buf, size_t size, 
                          const struct sockaddr_in& addr);

            void toIp(char* buf, size_t size,
                      const struct sockaddr_in& addr);

            void fromIpPort(const char* ip, uint16_t port,
                            struct sockaddr_in* addr);

            int  getSocketError(int sockfd);

            bool isSelfConnect(int sockfd);
            struct sockaddr_in getLocalAddr(int sockfd);
            struct sockaddr_in getPeerAddr(int sockfd);

        }// namespace sockets
    } // namespace net
} // namespace muduo

#endif // MUDUO_NET_SOCKETSOPS_H