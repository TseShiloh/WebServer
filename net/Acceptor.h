/*
Acceptor用于accept(2)接受TCP连接
Acceptor的数据成员包括Socket、Channel
Acceptor的socket是listening socket（即server socket）。
Channel用于观察此listening socket的readable事件，并回调Acceptor::handleRead()，后者调用accept(2)来接受新连接，并回调用户callback（应用层之上的）。

当一个客户端连接过来，listening socket会处于可读的状态，Channel会处于活跃的状态，Poller::poll()会调用该Channel::handleEvent()
*/

#ifndef MUDUO_NET_ACCEPTOR_H
#define MUDUO_NET_ACCEPTOR_H

#include <boost/function.hpp>
#include <boost/noncopuable.hpp>

#include <WebServer/net/Channel.h>
#include <WebServer/net/Socket.h>

namespace muduo
{
    namespace net
    {
        class EventLoop;
        class InetAddress;

        ///
        /// Acceptor of incoming TCP connections.
        ///
        class Acceptor : boost::noncopyable
        {
        private:
            void handleRead();

            EventLoop* loop_;// channel所属的EventLoop
            Socket acceptSocket_;// 监听套接字
            Channel acceptChannel_;// 观察套接字的可读事件
            NewConnectionCallback newConnectionCallback_;// 
            bool listenning_;// 是否处于监听的状态
            int idleFd_;// 
        
        public:
            typedef boost::function<void (int sockfd, const InetAddress&)> NewConnectionCallback;

            Acceptor(EventLoop* loop, const InetAddress& listenAddr);
            ~Acceptor();

            void setNewConnectionCallback(const NewConnectionCallback& cb) {
                newConnectionCallback_ = cb;
            }
            
            bool listenning() const { return listenning_; }
            void listen();

        }; // class Acceptor

    } // namespace net
    
} // namespace muduo

#endif // MUDUO_NET_ACCEPTOR_H