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
            TcpServer server_;
            HttpCallback httpCallback_;

            void onConnection(const TcpConnectionPtr& conn);
            void onMessage(const TcpConnectionPtr& conn,
                           Buffer* buf,
                           TimeStamp receiveTime);
            void onRequest(const TcpConnectionPtr&, const HttpRequest&);

        public:
            typedef boost::function<void (const HttpRequest&, HttpResponse*)> HttpCallback;
            
            HttpServer();
            ~HttpServer();
        };
        
    } // namespace net
    
} // namespace muduo


#endif MUDUO_NET_HTTP_HTTPSERVER_H