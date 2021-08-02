/*
Acceptor类的主要功能是socket、bind、listen
一般来说，在上层应用程序中，我们不直接使用Acceptor，而是把它作为TcpServer的成员
TcpServer还包含了一个TcpConnection列表
TcpConnection与Acceptor类似，有两个重要的数据成员，Socket与Channel

*/
#ifndef MUDUO_NET_TCPSERVER_H
#define MUDUO_NET_TCPSERVER_H

#include <WebServer/base/Types.h>
#include <WebServer/net/TcpConnection.h>

#include <map>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

namespace muduo
{
    namespace net
    {
        class Acceptor;
        class EventLoop;

        ///
        /// TCP server, supports single-threaded and thread-pool models.
        ///
        /// This is an interface class, so don't expose too much details.
        class TcpServer : boost::noncopyable
        {
        private:
            // Not thread safe, but in loop
            void newConnection(int sockfd, const InetAddress& peerAddr);// 连接到来
            /// Thread safe.
            void removeConnection(const TcpConnectionPtr& conn);

            typedef std::map<string, TcpConnectionPtr> ConnectionMap;// 连接列表<连接的名称,连接对象的指针>

            EventLoop* loop_;// acceptor_所属的EventLoop
            const string hostport_;// 服务端口
            const string name_;// 服务名
            // avoid revealing Acceptor
            boost::scoped_ptr<Acceptor> acceptor_;// 有了Acceptor类对象就有了socket,listen,bind的功能，避免暴露Acceptor
            ConnectionCallback connectionCallback_;// “连接到来”的回调函数
            MessageCallback messageCallback_;// “消息到来”的回调函数
            bool started_;// 是否已经启动了
            // always in loop thread
            int nextConnId_;// 下一个连接ID
            ConnectionMap connections_;// 连接列表

        public:
            //typedef boost::function<void(EventLoop*)> ThreadInitCallback;

            //TcpServer(EventLoop* loop, const InetAddress& listenAddr);
            TcpServer(EventLoop* loop,
                      const InetAddress& listenAddr,
                      const string& nameArg);
            ~TcpServer();  // force out-line dtor, for scoped_ptr members.

            const string& hostport() const { return hostport_; }
            const string& name()     const { return name_; }

            /// Starts the server if it's not listenning.
            ///
            /// It's harmless to call it multiple times.
            /// Thread safe.
            void start();

            /// Set connection callback.
            /// Not thread safe.
            // 设置“连接到来”或者“连接关闭”回调函数
            void setConnectionCallback(const ConnectionCallback& cb)
            { connectionCallback_ = cb; }

            /// Set message callback.
            /// Not thread safe.
            // 设置“消息到来”回调函数
            void setMessageCallback(const MessageCallback& cb)
            { messageCallback_ = cb; }

        }; // class TcpServer
            
    } // namespace net
    
} // namespace muduo


#endif  // MUDUO_NET_TCPSERVER_H