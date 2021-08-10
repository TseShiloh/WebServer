#ifndef MUDUO_NET_INSPECT_PROCESSINSPECTOR_H
#define MUDUO_NET_INSPECT_PROCESSINSPECTOR_H

#include <WebServer/net/inspect/Inspector.h>
#include <boost/noncopyable.hpp>
namespace muduo
{
    namespace net
    {
        class ProcessInspector : boost::noncopyable
        {
        private:
            static string pid(HttpRequest::Method, const Inspector::ArgList&);
            static string procStatus(HttpRequest::Method, const Inspector::ArgList&);
            static string openedFiles(HttpRequest::Method, const Inspector::ArgList&);
            static string threads(HttpRequest::Method, const Inspector::ArgList&);
        public:
            void registerCommands(Inspector* ins);  // 注册命令接口
        }; // class ProcessInspector
        
    } // namespace net
    
} // namespace muduo

#endif MUDUO_NET_INSPECT_PROCESSINSPECTOR_H