/*
HttpRequest：http请求类封装
HttpResponse：http响应类封装
HttpContext：http协议解析类
HttpServer：http服务器类封装
*/
#ifndef MUDUO_NET_HTTP_HTTPREQUEST_H
#define MUDUO_NET_HTTP_HTTPREQUEST_H

#include <WebServer/base/copyable.h>
#include <WebServer/base/Timestamp.h>
#include <WebServer/base/Types.h>

#include <map>
#include <assert.h>
#include <stdio.h>

namespace muduo
{
    namespace net
    {
        class HttpRequest : public muduo::copyable
        {
        private:
            Method method_;         // 请求方法
            Version version_;       // 协议版本1.0/1.1
            string path_;           // 请求路径
            TimeStamp receiveTime_; // 请求时间
            std::map<string, string> headers_;  // header列表

        public:
            enum Method// http方法不止这些，此处没有完全实现
            { kInvalid, kGet, kPost, kHead, kPut, kDelete};
            enum Version// http版本
            { kUnknown, kHttp10, kHttp11};

            HttpRequest()
                : method_(kInvalid),
                  version_(kUnknown)
            {}

            void setVersion(Version v)
            { version_ = v; }

            Version getVersion() const
            { return version_; }

            bool setMethod(const char* start, const char* end)
            {
                assert(method_ == kInvalid)
                stirng m(start, end);
                if (m == "GET")
                { method_ = kGet; }
                else if (m == "POST")
                { method_ = kPost; }
                else if (m == "HEAD")
                { method_ = kHead; }
                else if (m == "PUT")
                { method_ = kPut; }
                else if (m == "DELETE")
                { method_ = kDelete; }
                else
                { method_ = kInvalid; }
                return method_ != kInvalid;
            }

            Method method() const
            { return method_; }

            const char* methodString() const
            {
                const char* result = "UNKNOWN";
                switch(method_)
                {
                    case kGet:
                        result = "GET";
                        break;
                    case kPost:
                        result = "POST";
                        break;
                    case kHead:
                        result = "HEAD";
                        break;
                    case kPut:
                        result = "PUT";
                        break;
                    case kDelete:
                        result = "DELETE";
                        break;
                    default:
                        break;
                }
                return result;
            }

            void setPath(const char* start, const char* end)
            { path_.assign(start, end); }

            const string& path() const
            { return path_; }

            void setReceiveTime(Timestamp t)
            { receiveTime_ = t; }

            Timestamp receiveTime() const
            { return receiveTime_; }

            ///
            /// @param colon 冒号位置
            /// 
            void addHeader(const char* start, const char* colon, const char* end)
            {
                string field(start, colon); // header域
                ++colon;
                // 去除左空格
                while (colon < end && isspace(*colon)) {
                    ++colon;
                }
                string value(colon, end);   // header值
                // 去除右空格
                while (!value.empty() && isspace(value[value.size()-1])) {
                    value.resize(value.size()-1);
                }
                headers_[field] = value;
            }

            string getHeader(const string& field) const
            {
                string result;
                std::map<string, string>::const_iterator it = headers_.find(field);
                if (it != headers_.end()) {
                    result = it->second;
                }
                return result;
            }

            const std::map<string, string>& headers() const
            { return headers_; }

            void swap(HttpRequest& that)
            {
                std::swap(method_, that.method_);
                // std::swap(version_, that.version_);
                path_.swap(that.path_)
                receiveTime_.swap(that.receiveTime_);
                headers_.swap(that.headers_);
            }

        }; // class HttpRequest
    } // namespace net
} // namespace muduo


#endif  // MUDUO_NET_HTTP_HTTPREQUEST_H