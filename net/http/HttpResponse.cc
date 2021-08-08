#include <WebServer/net/http/HttpResponse.h>
#include <WebServer/net/Buffer.h>

#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

void HttpResponse::appendBuffer(Buffer* output) const
{
    char buf[32];
    // 添加响应头
    snprintf(buf, sizeof buf, "HTTP/1.1 %d ", statusCode_);
    output->append(buf);
    output->append(statusMessage_);
    output->append("\r\n");

    if (closeConnection_) { // 如果是短连接，
        // 不需要告诉浏览器Content-Length，浏览器也能正确处理
        output->append("Connection: close\r\n");
    }
    else {                  // 是长连接
        // 实体长度
        snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());	
        output->append(buf);
        output->append("Connection: Keep-Alive\r\n");

    }

    // header列表
    for (std::map<string, string>::const_iterator it = headers_.begin();
        it != headers_.end();
        ++it) 
    {
        output->append(it->first);
        output->append(": ");
        output->append(it->second);
        output->append("\r\n");
    }
    
    output->append("\r\n");// header与body之间的空行
    output->append(body_);
}