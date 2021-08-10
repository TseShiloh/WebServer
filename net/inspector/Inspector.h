#ifndef MUDUO_NET_INSPECT_INSPECTOR_H
#define MUDUO_NET_INSPECT_INSPECTOR_H

#include <Webserver/base/Mutex.h>
#include <WebServer/net/http/HttpRequest.h>
#include <WebServer/net/http/HttpServer.h>

#include <map>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

namespace muduo
{
    namespace net
    {
        class ProcessInspector;

        // A internal inspector of the running process, usually a singleton.
        class Inspector : boost::noncopyable
        {
        private:
            typedef std::map<string, Callback>  CommandList;    // <command, Callback> 输入相应的command，返回回调函数
            typedef std::map<string, string>    HelpList;       // <command, help>     输入相应的command，返回帮助信息

            void start();
            void onRequest(const HttpRequest& req, HttpResponse* resp);

            HttpServer server_;
            boost::scoped_ptr<ProcessInspector> processInspector_;
            MutexLock mutex_;
            std::map<string, CommandList>  commands_;   // <module, CommandList>  模块/命令列表
            std::map<string, HelpList>     helps_;      // <module, HelpList>     模块/帮助列表

        public:
            typedef std::vector<string> ArgList;
            typedef boost::function<string (HttpRequest::Method, const ArgList& args)> Callback;
            Inspector(Eventloop* loop,
                      const InetAddress& httpAddr,
                      const string& name);
            ~Inspector();

            // 如add("proc", "pid", ProcessInspector::pid, "print pid");
            // http://192.168.159.188:12345/proc/pid这个http请求就会相应的调用ProcessInspector::pid来处理
            void add(const string& module, 
                     const string& command, 
                     const Callback& cb, 
                     const string& help);

        }; // class Inspector

    } // namespace net
    
} // namespace muduo


#endif MUDUO_NET_INSPECT_INSPECTOR_H