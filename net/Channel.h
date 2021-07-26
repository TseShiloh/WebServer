/*
Channel类是对I/O事件的注册域响应的封装
Channel的成员函数update()负责注册或更新I/O的可读可写等事件 -> 调用了EventLoop的updateChannel() -> 调用了Poller的updateChannel()
Channel的成员函数handleEvent()负责对所发生的I/O事件进行处理
一个EventLoop可以对应多个Channel，即一对多。而且是聚合关系，EventLoop不负责Channel的生存期控制
Channel不拥有文件描述符，Channel对象销毁的时候不会调用close关闭文件描述符

*/

#ifndef MUDUO_NET_CHANNEL_H
#define MUDUO_NET_CHANNEL_H

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <muduo/base/Timestamp.h>

namespace muduo
{
    namespace net
    {
        class EventLoop;

        ///
        /// A selectable I/O channel.
        ///
        /// This class doesn't own the file descriptor.
        /// The file descriptor could be a socket,
        /// an eventfd, a timerfd, or a signalfd
        class Channel : boost::noncopyable
        {
        private:
            void update();
            void handleEventWithGuard(Timestamp receiveTime);

            static const int kNoneEvent;
            static const int kReadEvent;
            static const int kWriteEvent;

            EventLoop*  loop_;      // 所属EventLoop
            const int   fd_;        // 文件描述符，但不负责关闭该文件描述符
            int         events_;    // 关注的事件
            int         revents_;   // poll/epoll返回的事件
            int         index_;     // used by Poller.表示在poll的事件数组中的序号
            bool        logHup_;    // for POLLHUP

            boost::weak_ptr<void> tie_;// 负责生存期的控制
            bool tied_;
            bool eventHandling_;    // 是否处于处理事件中
            ReadEventCallback readCallback_;
            EventCallback writeCallback_;
            EventCallback closeCallback_;
            EventCallback errorCallback_;

        public:
            typedef boost::function<void()> EventCallback;// 事件的回调处理
            typedef boost::function<void(Timestmamp)> ReadEventCallback;// 读事件的回调处理

            Channel(Eveq* loop, int fd);// 一个Channel只能由一个EventLoop负责
            ~Channel();

            void handleEvent(Timestmamp receiveTime);
            void setReadCallback(const ReadEventCallback& cb)
            { readCallback_ = cb; }
            void setWriteCallback(const EventCallback& cb)
            { writeCallback_ = cb; }
            void setCloseCallback(const EventCallback& cb)
            { closeCallback_ = cb; }
            void setErrorCallback(const EventCallback& cb)
            { errorCallback_ = cb; }

            /// Tie this channel to the owner object managed by shared_ptr,
            /// prevent the owner object being destroyed in handleEvent.
            //void tie(const boost::shared_ptr<void>&);

            int fd() const { return fd_; }
            int events() const { return events_; }
            void set_revents(int revt) { revents_ = revt; }// used by pollers
            // int revents() const { return revents_; }
            bool isNoneEvent() const { return events_ == kNoneEvent; }

            void enableReading() { events_ |= kReadEvent; update(); }
            //void disableReading() { events_ &= ~kReadEvent; update(); }
            void enableWriting() { events_ |= kWriteEvent; update(); }
            void disableWriting() { events_ &= ~kWriteEvent; update(); }
            void disableAll() { events_ = kNoneEvent; update(); }// 不关注事件了
            bool isWriting() const { return events_ & kWriteEvent; }

            // for Poller
            int index() { return index_; }
            void set_index(int idx) { index_ = idx; }

            // for debug（转成字符串以便调试）
            string reventsToString() const;

            void doNotLogHup() { logHup_ = false; }

            EventLoop* ownerLoop() { return loop_; }
            void remove();
            
        }; // class Channel

    } // namespace net
    
} // namespace muduo


#endif // MUDUO_NET_CHANNEL_H