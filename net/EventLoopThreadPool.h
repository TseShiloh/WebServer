/*
EventLoopThreadPool（IO线程池类）
- IO线程池的功能是开启若干个IO线程，并让这些IO线程处于事件循环的状态
*/

#ifndef MUDUO_NET_EVENTLOOPTHREADPOOL_H
#define MUDUO_NET_EVENTLOOPTHREADPOOL_H

#include <vector>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace muduo
{
    namespace net
    {
        class EventLoop;
        class EventLoopThread;

        class EventLoopThreadPool : boost::noncopyable
        {
        private:
            EventLoop* baseLoop_;// 与Acceptor所属EventLoop相同
            bool started_;       // 是否启动 
            int  numThreads_;    // 线程数
            int  next_;          // 新连接到来，所选择的EventLoop对象的下标
            boost::ptr_vector<EventLoopThread> threads_;    // IO线程列表
            // ptr_vector对象threads_销毁时，所管理的EventLoopThread对象也会销毁
            std::vector<EventLoop*>            loops_;      // EventLoop列表
            // 一个IO线程对应一个EventLoop对象，都在栈上，不需要我们销毁

        public:
            typedef boost::function<void(EventLoop*)> ThreadInitCallback;

            EventLoopThreadPool(EventLoop* baseLoop);
            ~EventLoopThreadPool();
            void setThreadNum(int numThreads) { numThreads_ = numThreads; }
            void start(const ThreadInitCallback& cb = ThreadInitCallback());
            EventLoop* getNextLoop();
        
        };// class EventLoopThreadPool
    } // namespace net
} // namespace muduo

#endif  // MUDUO_NET_EVENTLOOPTHREADPOOL_H