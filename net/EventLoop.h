#ifndef MUDUO_NET_EVENTLOOP_H
#define MUDUO_NET_EVENTLOOP_H

#include <boost/noncopyable.hpp>

#include <muduo/base/CurrentThread.h>
#include <muduo/base/Thread.h>

namespace muduo
{
    namespace net
    {
        class Channel;
        class Poller;
        
        // EventLoop就是Reactor模式的封装，one per thread at most
        class EventLoop : boost::noncopyable
        {
        private:
            void abortNotInLoopThread();

            void printActiveChannels() const;

            bool looping_;          // 是否处于循环的状态; atomic
            bool quit_;             // 是否退出loop
            bool eventHandling_;    // 当前是否处于事件处理的状态

            const pid_t threadId_;  // 当前对象所属线程ID

            Timestamp pollReturnTime_;// 调用pool()函数时所返回的时间
            boost::scoped_ptr<Poller> poller_;// poller的生存期由EventLoop来控制
            
            ChannelList activeChannels_;    // Poller返回的活动通道
            Channel* currentActiveChannel_; // 当前正在处理的活动通道

        public：
            EventLoop();
            ~EventLoop();

            ///
            /// Loops forever
            /// Must be called in the same thread as creation of the object.
            ///
            void loop();// 事件循环

            void quit();

            ///
            /// Time when poll returns, usually means data arrival.
            ///
            Timestamp pollReturnTime() const { return pollReturnTime_; }

            //internal usage
            void updateChannel(Channel* channel);// 在Poller中添加或者更新通道
            void removeChannel(Channel* channel);// 从Poller中移除通道

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