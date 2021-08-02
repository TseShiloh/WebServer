#include <WebServer/net/Acceptor.h>

#include <WebServer/net/EventLoop.h>
#include <WebServer/net/InetAddress.h>
#include <WebServer/net/SocketsOps.h>

#include <boost/bind.hpp>

#include <errno.h>
#include <fcntl.h>

using namespace muduo;
using namespace muduo::net;

Acceptor::Acceptor()
    : loop_(loop),
      acceptSocket_(sockets::createNonblockingOrDie()),
      acceptChannel_(loop, acceptSocket_.fd()),
      listenning_(false),
      idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))// 预先准备了一个空闲的描述符
{
    assert(idleFd_ >= 0);
    acceptSocket_.setReuseAddr(true);// 设置地址重复利用
    acceptSocket_.bindAddress(listenAddr);// 绑定，但还没开始监听
    acceptChannel_.setReadCallback(
        boost::bind(&Acceptor::handleRead, this));// 设置一个“读”的回调函数
    
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
    ::close(idleFd_);
}

void Acceptor::listen()
{
    loop_->assertInLoopThread();
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

void Acceptor::handleRead()
{
    loop_->assertInLoopThread();
    InetAddress peerAddr(0);
    // FIXME loop until no more
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0)
    {
        // string hostport = peerAddr.toIpPort();
        // LOG_TRACE << "Accepts of " << hostport;
        if (newConnectionCallback_) {
            newConnectionCallback_(connfd, peerAddr);
        }
        else {
            sockets::close(connfd);
        }
    }
    else
    {
        if (errno == EMFILE) {
            // 文件描述符太多
            ::close(idleFd_);
            idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL);
            ::close(idleFd_);// 接受了马上关闭
            idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}