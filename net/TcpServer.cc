#include <WebServer/net/TcpServer.h>

#include <WebServer/base/Logging.h>
#include <WebServer/net/Acceptor.h>
#include <WebServer/net/EventLoop.h>
#include <WebServer/net/EventLoopThreadPool.h>

#include <WebServer/net/SocketsOps.h>

#include <boost/bind.hpp>

#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

TcpServer::TcpServer(EventLoop* loop,
                     const InetAddress& listenAddr,
                     const string& nameArg)
    : loop_(CHECK_NOTNULL(loop)),// 检查这个loop不是个空指针
      hostport_(listenAddr.toIpPort()),// 端口号
      name_(nameArg),
      acceptor_(new Acceptor(loop, listenAddr)),// 用智能指针scoped_ptr<Acceptor>来管理
      threadPool_(new EventLoopThreadPool(loop)),// 初始化,即是mainReactor，baseLoop_
      connectionCallback_(defaultConnectionCallback),// 声明在Callbacks.h
      messageCallback_(defaultMessageCallback),
      started_(false),// 是否启动
      nextConnId_(1)
{
    // Poller::poll() -> Channel::handleEvent() -> Acceptor::handleRead()
    // Acceptor::handleRead() -> TcpServer::newConnection()
    // _1：socket文件描述符， _2：对等方的地址（InetAddress）
    acceptor_->setNewConnectionCallback(
        boost::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer()
{
    loop_->assertInLoopThread();
    LOG_TRACE << "TcpServer::~TcpServer [" << name_ << "] destructing";

    for (ConnectionMap::iterator it(connections_.begin()); 
         it != connections_.end(); ++it)
    {
        TcpConnectionPtr conn = it->second;
        it->second.reset();		// 释放当前所控制的对象，引用计数减一
        conn->getLoop()->runInLoop(boost::bind(&TcpConnection::connectDestroyed, conn));
        conn.reset();			// 释放当前所控制的对象，引用计数减一
    }
}

void TcpServer::setThreadNum(int numThreads)
{
    assert(numThreads >= 0);
    threadPool_->setThreadNum(numThreads);// 设置线程池中的IO线程个数，
    // 但是不包含主EventLoop所属的线程，即IO线程总数 = numThreads + 1
}

// 该函数多次调用是无害的
// 该函数可以跨线程调用
void TcpServer::start()
{
    if (!started_) {
        started_ = true;
        threadPool_->start(threadInitCallback_);

    }

    if (!acceptor_->listenning()) {
	    // get_pointer返回原生指针
        loop_->runInLoop(
            boost::bind(&Acceptor::listen, get_pointer(acceptor_)));
    }
}

void TcpServer::newConnection(int sockfd, const InerAddress& peerAddr)
{
    loop_->assertInLoopThread();
    // 按照轮询的方式选择一个EventLoop
    EventLoop* ioLoop = threadPool_->getNextLoop();
    char buf[32];
    snprintf(buf, sizeof buf, ":%s#%d", hostport_.c_str(), nextConnId_);
    ++nextConnId_;
    string connName = name_ + buf;// 连接的名称

    LOG_INFO << "TcpServer::newConnection [" << name_
             << "] - new connection [" << connName;
             << '] from ' << peerAddr.toIpPort();
    InetAddress localAddr(sockets::getLocalAddr(sockfd));
    // FIXME poll with zero timeout to double confirm the new connection
    // FIXME use make_shared if necessary
    TcpConnectionPtr conn(new TcpConnection(ioLoop, 
                                            connName, 
                                            sockfd,
                                            localAddr, 
                                            peerAddr));

    LOG_TRACE << "[1] usecount=" << conn.use_count();
    connections_[connName] = conn;
    LOG_TRACE << "[2] usecount=" << conn.use_count();
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    conn->setCloseCallback(
        boost::bind(&TcpServer::removeConnection, this, _1));

    // conn->connectEstablished();// 在当前IO线程中调用（即loop_）
    ioLoop->runInLoop(
        boost::bind(&TcpConnection::connectEstablished, conn));

    LOG_TRACE << "[5] usecount=" << conn.use_count();

}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
	/*
  loop_->assertInLoopThread();
  LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
           << "] - connection " << conn->name();


  LOG_TRACE << "[8] usecount=" << conn.use_count();
  size_t n = connections_.erase(conn->name());
  LOG_TRACE << "[9] usecount=" << conn.use_count();

  (void)n;
  assert(n == 1);
  
  loop_->queueInLoop(
      boost::bind(&TcpConnection::connectDestroyed, conn));
  LOG_TRACE << "[10] usecount=" << conn.use_count();
  */

    loop_->runInLoop(
        boost::bind(&TcpServer::removeConnectionInLoop, this, conn));

}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
    loop_->assertInLoopThread();
    LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
             << "] - connection " << conn->name();


    LOG_TRACE << "[8] usecount=" << conn.use_count();
    size_t n = connections_.erase(conn->name());// 将对象conn从列表connections_中移除，引用计数-1
    LOG_TRACE << "[9] usecount=" << conn.use_count();

    (void)n;
    assert(n == 1);

    EventLoop* ioLoop = conn->getLoop();
    loop_->queueInLoop(
        boost::bind(&TcpConnection::connectDestroyed, conn));
    // 此处得到一个boost::function对象并将conn传递进去，因此引用计数+1
    
    LOG_TRACE << "[10] usecount=" << conn.use_count();
}
