#ifndef MUDUO_NET_CALLBACKS_H
#define MUDUO_NET_CALLBACKS_H

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include <WebServer/base/Timestamp.h>

namespace muduo
{
    namespace net
    {
        class TcpConnection;
        typedef boost::shared_ptr<TcpConnection> TcpConnectionPtr;

        typedef boost::function<void()> TimerCallback;

        typedef boost::function<void (const TcpConnectionPtr&)> ConnectionCallback;
        typedef boost::function<void (const TcpConnectionPtr&)> CloseCallback;

        // the data has been read to (buf, len)
        typedef boost::function<void (const TcpConnectionPtr&, Buffer*, Timestamp)> MessageCallback;

        void defaultConnectionCallback(const TcpConnectionPtr& conn);
        void defaultMessageCallback(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp receiveTime);
							
    } // namespace net
    
} // namespace muduo

#endif // MUDUO_NET_CALLBACKS_H