#ifndef MUDUO_NET_EVENTLOOP_H
#define MUDUO_NET_EVENTLOOP_H

#include <boost/noncopyable.hpp>

#include <muduo/base/CurrentThread.h>
#include <muduo/base/Thread.h>

namespace muduo
{
    namespace net
    {
        // EventLoop就是Reactor模式的封装，one per thread at most
        class EventLoop : boost::noncopyable
        {
        private:
            void abortNotInLoopThread();

            bool looping_;// 是否处于循环的状态; atomic
            const pid_t threadId_;// 当前对象所属线程ID

        public：
            EventLoop();
            ~EventLoop();

            ///
            /// Loops forever
            /// Must be called in the same thread as creation of the object.
            ///
            void loop();// 事件循环

            void assertInLoopThread() {
                if (!isInLoopThread()) {
                    abordNotInLoopThread();
                }
            }

            bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

            static EventLoop* getEventLoopOfCurrentThread();
            
        }; // class EventLoop
        
    } // namespace net
    
} // namespace muduo


#endif // MUDUO_NET_EVENTLOOP_H