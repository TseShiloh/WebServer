/*
HttpRequest：http请求类封装
HttpResponse：http响应类封装
HttpContext：http协议解析类
HttpServer：http服务器类封装
*/
#ifndef MUDUO_NET_HTTP_HTTPSERVER_H
#define MUDUO_NET_HTTP_HTTPSERVER_H

#include <WebServer/net/TcpServer.h>
#include <boost/noncopyable.hpp>

namespace muduo
{
    namespace net
    {
        class HttpRequest;
        class HttpResponse;

        class HttpServer : boost::noncopyable
        {
        private:
            TcpServer     server_;
            HttpCallback  httpCallback_; // 在处理http请求（即调用onRequest）的过程中回调此函数，对请求进行具体的处理

            void onConnection(const TcpConnectionPtr& conn);
            void onMessage(const TcpConnectionPtr& conn,
                           Buffer* buf,
                           TimeStamp receiveTime);
            void onRequest(const TcpConnectionPtr&, const HttpRequest&);

        public:
            typedef boost::function<void (const HttpRequest&, 
                                          HttpResponse*)> HttpCallback;
            
            HttpServer(EventLoop* loop,
                       const InetAddress& listenAddr,
                       const string& name);

            ~HttpServer();

            void start();

            /// Not thread safe, callback be registered before calling start().
            void setHttpCallBack(const HttpCallback& cb) {
                httpCallback_ = cb;
            }

            // 支持多线程
            void setThreadNum(int numThreads) {
                server_.setThreadNum(numThreads);
            }

        }; // class HttpServer
        
    } // namespace net
    
} // namespace muduo


#endif MUDUO_NET_HTTP_HTTPSERVER_H