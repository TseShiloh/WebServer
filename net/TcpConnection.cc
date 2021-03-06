#include <WebServer/net/TcpConnection.h>

#include <WebServer/base/Logging.h>
#include <WebServer/net/EventLoop.h>
#include <WebServer/net/Channel.h>
#include <WebServer/net/Socket.h>
#include <WebServer/net/SocketsOps.h>

#include <boost/bind.hpp>

#include <errno.h>
#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

void muduo::net::defaultConnectionCallback(const TcpConnectionPtr& conn)
{
    LOG_TRACE << conn->locaAddress().toIpPort() << " -> "
              << conn->peerAddress().toIpPort() << " is "
              << (conn->connected()? "UP" : "DOWN");
}

void muduo::net::defaultMessageCallback(const TcpConnectionPtr&,
                                        Buffer* buf,
                                        Timestamp)
{
    buf->retrieveAll();
}


TcpConnection::TcpConnection(EventLoop* loop,
                          const string& nameArg,
                          int sockfd,
                          const InetAddress& localAddr,
                          const InetAddress& peerAddr)
    : loop_(CHECK_NOTNULL(loop)),
      name_(nameArg),
      state_(kConnecting),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr)
{
    // 通道可读事件到来的时候，回调TcpConnection::handleRead()，_1是事件发生时间
    channel_->setReadCallbac (
        boost::bind(&TcpConnection::handleRead, this, _1);
    // 通道可写事件到来的时候，回调TcpConnection::handleWrite
    channel_->setWriteCallback(
        boost::bind(&TcpConnection::handleWrite, this)); 
    // 连接关闭，回调TcpConnection::handleClose()
    channel_->setCloseCallback(
        boost::bind(&TcpConnection::handleClose, this));
    // 发生错误，回调TcpConnection::handleError()
    channel_->setErrorCallback(
        boost::bind(&TcpConnection::handleError, this));

    LOG_DEBUG << "TcpConnection::ctor[" << name_ << "] at" 
              << this; << " fd=" << sockfd;
    socket_->setKeepAlive(true);
}
                          
                          
                          
TcpConnection::~TcpConnection()
{
    LOG_DEBUG << "TcpConnection::dtor[" << name_ << "] at" << this
              << " fd=" << channel_->fd();
}

// 线程安全，可以跨线程调用
void TcpConnection::send(const void* data, size_t len)
{
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {// 本线程调用
            sendInLoop(data, len);
        }
        else {// 跨线程调用
            string message(static_cast<const char*>(data), len);
            loop_->runInLoop(
                boost::bind(&TcpConnection::sendInLoop,
                            this,
                            message));
        }
    }
}

// 线程安全，可以跨线程调用
void send(const StringPiece& message)
{
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {// 本线程调用
            sendInLoop(message);
        }
        else {// 跨线程调用
            loop_->runInLoop(
                boost::bind(&TcpConnection::sendInLoop,
                            this,
                            message.as_string()));
                            //std::forward<string>(message)));
        }
    }
}

// 线程安全，可以跨线程调用
void TcpConnection::send(Buffer* buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread()) {
        sendInLoop(buf->peek(), buf->readableBytes());
        buf->retrieveAll();
        }
        else {
        loop_->runInLoop(
            boost::bind(&TcpConnection::sendInLoop,
                        this,
                        buf->retrieveAllAsString()));
                        //std::forward<string>(message)));
        }
    }
}

void TcpConnection::sendInLoop(const StringPiece& message)
{
    sendInLoop(message.data(), message.size());
}

void TcpConnection::sendInLoop(const void* data, size_t len)
{
    loop_->assertInLoopThread();
    // sockets::write(channel_->fd(), data, len);
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool error = false;
    if (state_ == kDisconnected) {
        LOG_WARN << "disconnected, give up writing";
        return;
    }
    // if no thing in output queue, try writing directly
    // 通道没有关注可写事件并且发送缓冲区没有数据，直接write
    if (!channel_->isWriting() && outputbuffer_.reableBytes() == 0) {
        nwrote = sockets::write(channel_->fd(), data, len);
        if (nwrote >= 0) {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_) {// 写完了，回调writeCompleteCallback_
                loop_->queueInLoop(boost::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
    }
    assert(remaining <= len);
    if (!error &&  remaining > 0) { // 没有错误，并且还有没写完的数据（说明内核发送缓冲区满，要将未写完的数据添加到output buffer中）
        LOG_TRACE << "I am going to write more data."
        size_t oldLEN = outputBuffer_.readableBytes();// 目前output buffer中的数据
        // 如果超过highWaterMark_（高水位标），回调highWaterMarkCallback_
        if (oldLen + remaining >= highWaterMark_
            && oldLen < highWaterMark_
            && highWaterMarkCallback_)
        {
            loop_->queueInLoop(boost::bind(highWaterMarkCallback_, 
                                           shared_from_this(), 
                                           oldLen + remaining));
        }
        outputBuffer_.append(static_cast<const char*>(data)+nwrote, remaining);// 写入起始的位置为data偏移nwrote后，长度为remaining
        if (!channel_->isWriting()) {
            channel_->enableWriting(); // 关注POLLOUT事件
        }
    }
}

void TcpConnection::shutdown()
{
    // FIXME: use compare and swap
    if (state_ == kConnected)// 没有保护这个状态，可以用原子性操作
    {
        setState(kDisconnecting);
        loop_->runInLoop(boost::bind(&TcpConnection::shutdownInLoop, this));
        // FIXME: this -> shared_from_this() ?
    }
}

void TcpConnection::shutdownInLoop()
{
    loop_->assertInLoopThread();
    if (!channel_->isWriting())// 如果还有数据没发送完，那么只是将状态改为“kDisconnecting”，并没有关闭连接
    {
        // we are not writing
        socket_->shutdownWrite();// 关闭“写”
    }
}

void TcpConnection::setTcpNoDelay(bool on)
{
    socket_->setTcpNoDelay(on);
}

void TcpConnection::connectEstablished()
{
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);
    setState(kConnected);
    LOG_TRACE << "[3] usecount=" << shared_from_this().use_count();
    channel_->tie(shared_from_this());// 跟TcpConnection的生存期有关
    channel_->enableReading();// TcpConnection所对应的通道加入到Poller关注

    connectionCallback_(shared_from_this());// 用户的回调函数connectionCallback_
    LOG_TRACE << "[4] usecount=" << shared_from_this().use_count();

}

void TcpConnection::connectDestroyed()
{
    loop_->assertInLoopThread();
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll();

        connectionCallback_(shared_from_this());
    }
    channel_->remove();// 该通道从poll当中移除
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    loop_->assertInLoopThread();
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0) {
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
        // shared_from_this将裸指针转换成shared_ptr
    }
    else if (n == 0) {
        handleClose();
    }
    else {
        errno = savedErrno;
        LOG_SYSERR << "TcpConnection::handleRead";
        handleError();
    }
}

void TcpConnection::handleWrite() // 内核发送缓冲区有空间了，回调该函数
{
    loop_->assertInLoopThread();
    if (channel_->isWriting()) {
        ssize_t n  = sockets::write(channel_->fd(),
                                    outputBuffer_.peak(),
                                    outputBuffer_.read)
        if (n > 0) {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0) {// 发送缓冲区已经清空
                channel_->disableWriting();          // 停止关注可写事件，以免出现busy loop
                if (writeCompleteCallback_) {        // 回调writeCompleteCallback_
                    // 应用层发送缓冲区被清空，就回调用writeCompleteCallback_
                    loop_->queueInLoop(boost::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting)// 发送缓冲区已清空并且连接状态是kDisconnecting，则要关闭连接
                {
                    shutdownInLoop();// 关闭连接
                }
            }
            else {
                LOG_TRACE << "I am going to write more data.";
            }
        }
    }
}

void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();
    LOG_TRACE << "fd = " << channel_->fd() << " state = " << state_;
    assert(state_ == kConnected || state_ == kDisconnecting);
    // we don't close fd, leave it to dtor, so we can find leaks easily.
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr guardThis(shared_from_this());
    connectionCallback_(guardThis);		// 这一行，可以不调用
    LOG_TRACE << "[7] usecount=" << guardThis.use_count();
    // must be the last line
    closeCallback_(guardThis);	// 调用TcpServer::removeConnection
    LOG_TRACE << "[11] usecount=" << guardThis.use_count();
}

void TcpConnection::handleError()
{
    int err = sockets::getSocketError(channel_->fd());
    LOG_ERROR << "TcpConnection::handleError [" << name_
              << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}
