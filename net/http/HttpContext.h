/*
HttpRequest：http请求类封装
HttpResponse：http响应类封装
HttpContext：http协议解析类
HttpServer：http服务器类封装
*/

#ifndef MUDUO_NET_HTTP_HTTPCONTEXT_H
#define MUDUO_NET_HTTP_HTTPCONTEXT_H

#include <WebServer/base/copyable.h>
#include <WebServer/net/http/HttpRequest.h>
namespace muduo
{
    namespace net
    {
        class HttpContext : public WebServer::copyable
        {
        private:
            HttpRequestParseState   state_;     // 请求解析状态
            HttpRequest             request_;   // http请求

        public:
            enum HttpRequestParseState
            {
                kExpectRequestLine,     // 解析请求行的状态
                kExpectHeaders,         // 解析头部信息的状态
                kExpectBody,            // 解析实体的状态
                kGotAll,                // 全部解析完毕
            };

            HttpContext()
                : state_(kExpectRequestLine) // 初始状态：希望收到的是请求行
            {}

            // default copy-ctor, dtor and assignment are fine
            bool expectRequestLine() const
            { return state_ == kExpectRequestLine; }

            bool expectHeaders() const
            { return state_ == kExpectHeaders; }

            bool expectBody() const
            { return state_ == kExpectBody; }

            bool gotAll() const
            { return state_ == kGotAll; }

            void receiveRequestLine()    // 请求函已经接收完毕
            { state_ = kExpectHeaders; } // 下一个希望接受的是Headers

            void receiveHeaders()       // Headers已经接收完毕
            { state_ = kGotAll; }       // 当前没有处理Body，所以直接是kGotAll

            // 重置HttpContext状态
            void reset()
            {
                state_ = kExpectRequestLine;
                HttpRequest dummy;      // 新建一个空HttpRequest对象
                request_.swap(dummy);   // 将当前对象request_置空
            }

            const HttpRequest& request() const
            { return request_; }

            HttpRequest& request()
            { return request_; }


        };
    } // namespace net
    
} // namespace muduo


#endif  // MUDUO_NET_HTTP_HTTPCONTEXT_H