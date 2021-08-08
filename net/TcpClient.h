#ifndef MUDUO_NET_TCPCLIENT_H
#define MUDUO_NET_TCPCLIENT_H

#include <boost/noncopyable.hpp>

#include <WebServer/base/Mutex.h>
#include <WebServer/net/TcpConnection.h>

namespace muduo
{
    namespace net
    {
        class Connector;
        typedef boost::shared_ptr<Connector> ConnectorPtr;
        
        class TcpClient : boost::noncopyable
        {
        private:
            /// Not thread safe, but in loop
            void newConnection(int sockfd);
            void removeConnection(const TcpConnectionPtr& conn);

            EventLoop* loop;
            Connector connector_;   // 用于发起主动连接
            const string name_;     // 名称
            ConnectionCallback connectionCallback_;         // 连接建立回调函数
            MessageCallback messageback_;                   // 消息到来回调函数
            WriteCompleteCallback writeCompleteCallback_;   // 数据发送完毕回调函数
            bool retry_;    // 重连，是指连接建立之后又断开的时候是否重连
            bool connect_;  // atomic
            // always in loop thread
            int nextConnId_;// name_ + nextConnId_用于标识一个连接
            mutable MutexLock mutex_;
            TcpConnectionPtr connection_;// Connector连接成功以后，得到一个Connection

        public:
            TcpClient(EventLoop* loop,
                      const InetAddress& serverAddr,
                      const string& name);
            ~TcpClient();// force out-line dtor, for scoped_ptr members.

            void connect();
            void disconnect();
            void stop();

            TcpConnectionPtr connection() const
            {
                MutexLockGuard lock(mutex_);
                return connection_;
            }

            bool retry() const;
            void enableRetry() { }

            /// Set connection callback.
            /// not thread safe.
            void setConnectionCallback(const ConnectionCallback& cb)
            { connectionCallback_ = cb; }

            void setMessageCallback(const MessageCallback& cb)
            { messageback_ = cb; }

            void setWriteCompleteCallback(const WriteCompleteCallback& cb)
            { writeCompleteCallback_ = cb; }

        };
        
        TcpClient::TcpClient(/* args */)
        {
        }
        
        TcpClient::~TcpClient()
        {
        }
        
    } // namespace net
    
} // namespace muduo
