#include <WebServer/net/http/HttpServer.h>

#include <WebServer/base/Logging.h>
#include <WebServer/net/http/HttpContext.h>
#include <WebServer/net/http/HttpRequest.h>
#include <WebServer/net/http/HttpResponse.h>

#include <boost/bind.hpp>

using namespace muduo;
using namespace muduo::net;

namespace muduo
{
    namespace net
    {
        namespace detail
        {
            // FIXME: move to HttpContext class
            bool processRequestLine(const char* begin, const char* end, HttpContext* context)
            {
                bool succeed = false;
                const char* start = begin;
                const char* space = std::find(start, end, ' ');
                HttpRequest& request = context->request();     // 取出请求对象
                if (space != end && request.setMethod(start, space)) {   // 解析请求方法
                    start = space + 1;
                    space = std::find(start, end, ' ');
                    if (space != end) {
                        request.setPath(start, space);  // 解析PATH
                        start = space + 1;
                        succeed = end - start == 8 && std::equal(start, end - 1, "HTTP/1.");
                        if (succeed) {
                            if (*(end-1) == '1') {
                                request.setVersion(HttpRequest::kHttp11);   // HTTP/1.1
                            }
                            else if (*(end-1) == '0') {
                                request.setVersion(HttpRequest::kHttp10);   // HTTP/1.0
                            }
                            else {
                                succeed = false;
                            }
                        }
                    }
                }
                return succeed;
            }

            // FIXME: move to HttpContext class
            // return false if any error
            bool parseRequest(Buffer* buf, HttpContext* context, Timestamp receiveTime)
            {
                bool ok = true;
                bool hasMore = true;
                while (hasMore)
                {
                    if (context->expectRequestLine()) { // 处于解析请求行状态
                        const char* crlf = buf->findCRLF();// 首先查找/r/n，并指向/r的位置
                        if (crlf) { // 查找到了/r/n
                            ok = processRequestLine(buf->peek(), crlf, context);    // 解析请求行
                            if (ok) {
                                context->request().setReceiveTime(receiveTime); // 设置请求时间
                                buf->retrieveUntil(crlf + 2);   // 将请求行从buf中取回，包括/r/n
                                context->receiveRequestLine();  // HttpContext将状态改为kExpectHeaders
                            }
                            else {
                                hasMore = false;
                            }
                        }
                        else {
                            hasMore = false;
                        }
                    }
                    else if (context->expectHeaders()) {    // 解析Header
                        const char* crlf = buf->findCRLF();
                        if (crlf) { // 查找到了/r/n
                            const char* colon = std::find(buf->peek(), crlf, ':');  // 冒号所在位置
                            if (colon != crlf) {
                                context->request().addHeader(buf->peek(), colon, crlf);
                            }
                            else { // empty line, end of header
                                context->receiveHeaders();      // HttpContext将状态改为kGotAll
                                hasMore = !context->gotAll();
                            }
                            buf->retrieveUntil(crlf + 2);   // 将Header从buf中取回，包括/r/n
                        }
                        else {
                            hasMore = false;
                        }
                    }
                    else if (context->expectBody()) {   // 当前还不支持请求中带body
                        // FIXME;
                    }
                }
                return ok;
            }

            void defaultHttpCallback(const HttpRequest&, HttpResponse* resp)
            {
                resp->setStatusCode(HttpResponse::k404NotFound);
                resp->setStatusMessage("Not Found");
                resp->setCloseConnection(true);
            }


        } // namespace detail

    } // namespace net

} // namespace muduo

HttpServer::HttpServer(EventLoop* loop,
                       const InetAddress& listenAddr,
                       const string& name)
    : server_(loop, listenAddr, name),
      httpCallback_(detail::defaultHttpCallback)
{
    server_.setConnectionCallback(
        boost::bind(&HttpServer::onConnection, this, _1));
    server_.setMessageCallback(
        boost::bind(&HttpServer::onMessage, this, _1, _2, _3));
}

HttpServer::~HttpServer()
{}

void HttpServer::start()
{
    LOG_WARN << "HttpServer[" << server_.name()
             << "] starts listening on" << server_.hostport();
    server_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr &conn)
{
    if (conn->connected()) {
        conn->setContext(HttpContext()); // TcpConnection与一个HttpContext绑定
    }
}

void HttpServer::onMessage(const TcpConnectionPtr &conn,
               Buffer *buf,
               TimeStamp receiveTime)
{
    // 取出http上下文
    HttpContext* context = boost::any_cast<HttpContext>(conn->getMutableContext());

    if (!detail::parseRequest(buf, context, receiveTime)) {
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        conn->shutdown();
    }

    // 请求消息解析完毕
    if (context->gotAll()) {
        onRequest(conn, context->request());
        context->reset();   // 本次请求处理完毕，重置HttpContext，适用于长连接
    }
}

void HttpServer::onRequest(const TcpConnectionPtr& conn, const HttpRequest& req)
{
    const string& connection = req.getHeader("Connection");
    bool close = connection == "close" || 
        (req.getVersion() == HttpRequest::kHttp10 && connection != "Keep-Alive");
    HttpResponse response(close);
    httpCallback_(req, &response);// 回调用户函数，对这个httpRequest进行相应的处理，并且返回一个response对象
    Buffer buf;
    response.appendToBuffer(&buf);// 将response对象转换成字符串添加到缓冲区buf当中
    conn->send(&buf);// 将缓冲区发送给客户端
    if (response.closeConnection()) {
        conn->shutdown();
    }
}

