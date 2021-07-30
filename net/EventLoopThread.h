/*
1. 任何一个线程，只要创建并运行了EventLoop，都称之为IO线程
2. IO线程不一定是主线程
3. muduo并发模型one loop per thread + threadpool

4. 为了方便今后使用，定义了EventLoopThread类，该类封装了IO线程
    - EventLoopThread创建了一个线程
    - 在线程函数中创建了一个EvenLoop对象并调用EventLoop::loop
*/
#ifndef MUDUO_NET_EVENTLOOPTHREAD_H
#define MUDUO_NET_EVENTLOOPTHREAD_H

#include <WebServer/base/Condition.h>
#include <WebServer/base/Mutex.h>
#include <WebServer/base/Thread.h>

#include <boost/noncopyable.hpp>

namespace muduo
{
    namespace net
    {
        class EventLoop;

        class EventLoopThread : boost::noncopyable
        {
        private:
            void threadFunc();// 线程函数，会在线程中创建一个EventLoop对象

            EventLoop* loop_;// loop_指向一个EventLoop对象（即上面的对象）
            Thread thread_;// muduo是基于对象的编程思想，所以拥有（而不是继承）一个Thread类对象
            bool exiting_;
            MutexLock mutex_;// 互斥量
            Condition cond_;// 条件变量
            ThreadInitCallback callback_;// 回调函数，在EventLoop::loop事件循环之前被调用

        public:
            typedef boost::function<void(EventLoop*)> ThreadInitCallback;// 默认是一个空的回调函数

            EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback());
            ~EventLoopThread();
            EventLoop* startLoop();// 启动线程，该线程就成为了IO线程
        };

        
    } // namespace net
} // namespace muduo

#endif // MUDUO_NET_EVENTLOOPTHREAD_H