/*
2.SocketsOps.h/ SocketsOps.cc
- 封装了socket相关系统调用（全局函数，位于muduo::net::sockets名称空间中）.
*/
#include <WebServer/net/SocketsOps.h>

#include <WebServer/base/Logging.h>
#include <WebServer/base/Types.h>
#include <WebServer/net/Endian.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>      // snprintf
#include <strings.h>    // bzero
#include <sys/socket.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

namespace
{
    typedef struct sockaddr SA;

    // 将网际地址sockaddr_in*转换成通用地址sockaddr*（const版）
    const SA* sockaddr_cast(const struct sockaddr_in* addr)
    {
        return static_cast<const SA*>(implicit_cast<const void*>(addr));
    }
    // 将网际地址sockaddr_in*转换成通用地址sockaddr*（无const版）
    SA* sockaddr_cast(struct sockaddr_in* addr)
    {
        return static_cast<SA*>(implicit_cast<void*>(addr));
    }

    void setNonBlockAndCloseOnExec(int sockfd)// 将这个文件描述符设置为非阻塞模式
    {
        // non-block
        int flags = ::fcntl(sockfd, F_GETFL, 0);
        flags |= O_NONBLOCK;
        int ret = ::fcntl(sockfd, F_SETFL, flags);

        // close-on-exec
        flags = ::fcntl(sockfd, F_GETFD, 0);
        flags |= O_CLOEXEC;
        ret = ::fcntl(sockfd, F_SETFD, flags);

        (void)ret;
    }
}

int sockets::createNonblockingOrDie()
{
    // socket
#if VALGRIND// valgrind内存检测工具，e.g. 内存泄漏情况、文件描述符的状态
    int sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0) {
    LOG_SYSFATAL << "sockets::createNonblockingOrDie";// 登记日志并终止程序
    }
    setNonBlockAndCloseOnExec(sockfd);
#else
    // Linux 2.6.27以上的内核支持SOCK_NONBLOCK与SOCK_CLOEXEC
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0) {
        LOG_SYSFATAL << "sockets::createNonblockingOrDie";
    }
#endif
    return sockfd;
}

void sockets::bindOrDie(int sockfd, const struct sockaddr_in& addr)
{
    int ret = ::bind(sockfd, sockaddr_cast(&addr), sizeof addr);
    if (ret < 0) {
        LOG_SYSFATAL << "sockets::bindOrDie";
    }
}

void sockets::listenOrDie(int sockfd)
{
    int ret = ::listen(sockfd, SOMAXCONN);
    if (ret < 0) {
        LOG_SYSFATAL << "sockets::listenOrDie";
    }
}

int sockets::accept(int sockfd, struct sockarr_in* addr)
{
    socklen_t addrlen = sizeof *addr;
#if VALGRIND// valgrind内存检测工具，e.g. 内存泄漏情况、文件描述符的状态
    int connfd = ::accept(sockfd, sockaddr_cast(addr), &addrlen);
    setNonBlockAndCloseOnExec(connfd);
#else
    // Linux 2.6.27以上的内核支持SOCK_NONBLOCK与SOCK_CLOEXEC
    int connfd = ::accept4(sockfd, sockaddr_cast(addr), &addrlen,
                           SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif
    if (connfd < 0) {
        int savedErrno = errno;// 先保存errno值，因为系统调用和库函数可能会改变该值。
        LOG_SYSERR << "Socket::accept";
        switch (savedErrno)
        { 
        case EAGAIN:
        case ECONNABORTED:
        case EINTR:
        case EPROTO: // ???
        case EPERM:
        case EMFILE: // per-process lmit of open file desctiptor ???
        // expected errors非致命的错误
            errno = savedErrno;    
            break;

        case EBADF:
        case EFAULT:
        case EINVAL:
        case ENFILE:
        case ENOBUFS:
        case ENOMEM:
        case ENOTSOCK:
        case EOPNOTSUPP:
        // unexpected errors致命的错误
            LOG_FATAL << "unexpected error of ::accept" << savedErrno;
            break;
        
        default:
        // unknown error未知的错误
            LOG_FATAL << "unknown error of ::accept " << savedErrno;
            break;
        }
    }
    return connfd;
}

int sockets::connect(int sockfd, const struct sockaddr_in& addr)
{
    return ::connect(sockfd, sockaddr_cast(&addr), sizeof addr);
}

ssize_t sockets::read(int sockfd, void *buf, size_t count)
{
    return ::read(sockfd, buf, count);
}

/** 
 * @brief readv与read不同之处在于，接收的数据可以填充到多个缓冲区中
 * @param sockfd socket文件描述符
 * @param iov 数组 
 * @param iovcnt 数组的元素个数
 * 
*/
ssize_t sockets::readv(int sockfd, const struct iovec *iov, int iovcnt)
{
    return ::readv(sockfd, iov, iovcnt);
}

// 相同的，write函数也有writev版本，可以将多个缓冲区的数据发送出去
ssize_t sockets::write(int sockfd, const void *buf, size_t count)
{
    return ::write(sockfd, buf, count);
}

// 关闭文件描述符
void sockets::close(int sockfd)
{
    if (::close(sockfd) < 0) {
        LOG_SYSERR << "sockets::close";
    }
}

// 只关闭“写”的这一半
void sockets::shutdownWrite(int sockfd)
{
    if (::shutdown(sockfd, SHUT_WR) < 0) {
        LOG_SYSERR << "sockets::shutdownWrite";
    }
}

// 将地址addr转换成IP与端口的形式保存在缓冲区buf当中
void sockets::toIpPort(char* buf, size_t size,
                       const struct sockaddr_in& addr)
{
    char host[INET_ADDRSTRLEN] = "INVALID";// INET_ADDRSTRLEN是一个宏（表示网际地址的长度）
    toIp(host, sizeof host, addr);// 将网际地址的IP放到host当中
    uint16_t port = sockets::networkToHost16(addr.sin_port);
    snprintf(buf, size, "%s:%u", host, port);
}

void sockets::toIp(char* buf, size_t size,
                   const struct sockarr_in& addr)
{
    assert(size >= INET_ADDRSTRLEN);// 缓冲区的大小一定要 ≥ 网际地址长度的大小
    ::inet_ntop(AF_INET, &addr.sin_addr, buf, static_cast<socklen_t>(size));
    // AF_INET表示IPv4协议；将sin_addr（32位的整数）转换为点分十进制的IP地址放入buf中
}

// 通过IP和端口构建网际地址sockaddr_in*
void sockets::fromIpPort(const char* ip, uint16_t port,
                        struct sockaddr_in* addr)
{
    addr->sin_family = AF_INET;// 支持IPv4
    addr->sin_port = hostToNetwork16(port);// 端口转换成16位网络字节序
    if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) {// muduo库并没有支持IPv6
        LOG_SYSERR << "sockets::fromIpPort";
    }
}

int sockets::getSocketError(int sockfd)
{
    int optval;
    socklen_t optlen = sizeof optval;

    if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        return errno;
    }
    else {
        return optval;
    }
}

struct sockaddr_in sockets::getLocalAddr(int sockfd)
{
    struct sockaddr_in localaddr;
    bzero(&localaddr, sizeof localaddr);
    socklen_t addrlen = sizeof(localaddr);
    if (::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen) < 0)// 获取本地地址
    {
        LOG_SYSERR << "sockets::getLocalAddr";
    }
    return localaddr;
}

struct sockaddr_in sockets::getPeerAddr(int sockfd)
{
    struct sockaddr_in peeraddr;
    bzero(&peeraddr, sizeof peeraddr);
    socklen_t addrlen = sizeof(peeraddr);
    if (::getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen) < 0)// 获取对等方地址
    {
        LOG_SYSERR << "sockets::getPeerAddr";
    }
    return peeraddr;
}

// 自连接是指(sourceIP, sourcePort) = (destIP, destPort)
// 自连接发生的原因:
// 客户端在发起connect的时候，没有bind(2)
// 客户端与服务器端在同一台机器，即sourceIP = destIP，
// 服务器尚未开启，即服务器还没有在destPort端口上处于监听
// 就有可能出现自连接，这样，服务器也无法启动了

bool sockets::isSelfConnect(int sockfd)
{
    struct sockaddr_in localaddr = getLocalAddr(sockfd);
    struct sockaddr_in peeraddr = getPeerAddr(sockfd);
    return localaddr.sin_port == peeraddr.sin_port
        && localaddr.sin_addr.s_addr == peeraddr.sin_addr.s_addr;
}