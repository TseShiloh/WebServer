#include <WebServer/net/TcpClient.h>

#include <WebServer/base/Logging.h>
#include <WebServer/net/Connector.h>
#include <WebServer/net/EventLoop.h>
#include <WebServer/net/SocketsOps.h>

#include <boost/bind.hpp>

#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

namespace muduo
{
    namespace net
    {
        namespace detail
        {
            void removeConnection(EventLoop* loop, const TcpConnectionPtr& conn)
            {
                loop->queueInLoop(boost::bind(&TcpConneciton::connectionDestroyed, conn));
            }
            
            void removeConnector(const ConnectorPtr& connector)
            {
                // connector->
            }
        } // namespace detail
        
    } // namespace net
    
} // namespace muduo

TcpClient::TcpClient(EventLoop* loop,
                    const InetAddress& serverAddr,
                    const string& name)
    : loop_(CHECK_NOTNULL(loop)),
      connector_(new Connector(loop, serverAddr)),
      name_(name),
      connectionCallback_(defaultConnectionCallback),
      messageCallback_(defaultMessageCallback),
      retry_(false),
      connect_(true),
      nextConnId_(1)
{
    // 设置连接成功回调函数
    connector_->setNewConnectionCallback(
        boost::bind(&TcpClient::newConnection, this, _1));
    // FIXME: setConnectFailedCallback
    LOG_INFO << "TcpClient::TcpClient[" << name_
             << "] - connector " << get_pointer(connector_);

}

TcpClient::~TcpClient()
{
    LOG_INFO << "TcpClient::~TcpClient[" << name_
            << "] - connector " << get_pointer(connector_);
    TcpConnectionPtr conn;
    {
        MutexLockGuard lock(mutex_);
        conn = connection_;
    }
    if (conn) {
        // 重新设置TcpConnection中的closeCallback_为detail::removeConnection
        // 因为TcpClient::removeConnection，因为会重连。而析构的时候是不再需要重连了
        CloseCallback cb = boost::bind(&detail::removeConnection,)
        loop_->runInLoop(
            boost::bind(&TcpConnection::setCloseCallback, conn, cb));
    }
    else {// 说明connector处于未连接状态，将connector_停止
        connector_->stop();
        loop_->runAfter(1, boost::bind(&detail::removeConnector, connector_));
    }
}

void TcpClient::connect()
{
    LOG_INFO << "TcpClient::connect[" << name_ << "] - connecting to "
             << connector_->serverAddress().toIpPort();
    connect_ = true;
    connector_->start();// 发起连接
}

// 用于连接已建立的情况下，关闭连接
void TcpClient::disconnect()
{
    connect_ = false;

    {
        MutexLockGuard lock(mutex_);
        if (connection_)
        {
            connection_->shutdown();
        }
    }
}

// 停止connector_(可能连接尚未建立成功)
void TcpClient::stop()
{
    connect_ = false;
    connector_->stop();
}

void TcpClient::newConneciton(int sockfd)
{
    loop_->assertInLoopThread();
    InetAddress peerAddr(sockets::getPeerAddr(sockfd));
    cahr buf[32];
    snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
    ++nextConnId_;
    string connName = name_ + buf;

    InetAddress localAddr(sockets::getLocalAddr(sockfd));

    TcpConnectionPtr conn(new TcpConnection(loop_,
                                            connName,
                                            sockfd,
                                            localAddr,
                                            peerAddr));

    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
        boost::bind(&TcpClient::removeConnection, this, _1));
    {
        MutexLockGuard lock(mutex_);
        connection_ =conn;// 保存TcpConnection
    }
    conn->connectEstablished();// 这里回调connectionCallback_
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
    loop_->assertInLoopThread();
    assert(loop_ == conn->getLoop());

    {
        MutexLockGuard lock(mutex_);
        assert(connection_ == conn);
        connection_.reset();
    }

    loop_->queueInLoop(boost::bind(&TcpConnection::connectDestroyed, conn));
    if (retry_ && connect_) {
        LOG_INFO << "TcpClient::connect[" << name_ << "] - Reconnecting to"
                 << connector_->serverAddress().toIpPort();
        // 这里的重连是指连接建立成功之后北段开的重连
        connector_->restart();
    }
}
