#ifndef MUDUO_NET_POLLER_H
#define MUDUO_NET_POLLER_H

#include <boost/noncopyable.hpp>

#include <WebServer/base/Timestamp.h>
#include <WebServer/net/EventLoop.h>
#include <vector>

namespace muduo
{
    namespace net
    {
        class Channel;

        class Poller : boost::noncopyable
        {
        private:
            EventLoop* ownerLoop_;// Poller所属的EventLoop，一个EventLoop包含一个Poller
        
        public:
            typedef std::vector<Channel*> ChannelList;

            Poller(EventLoop* loop);
            virtual ~Poller();

            /// Polls the I/O events.
            /// Must be called in the loop thread.
            virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;

            /// Changes the interested I/O events.
            /// Must be called in the loop thread.
            virtual void updateChannel(Channel* channel) = 0;

            /// Remove the channel, when it destructs.
            /// Must be called in the loop thread.
            virtual void removeChannel(Channel* channel) = 0;

            static Poller* newDefaultPoller(EventLoop* loop);

            void assertInLoopThread() {
                ownerLoop_->assertInLoopThread();
            }

        }; // class Poller

    } // namespace net

} // namespace muduo 


#endif // MUDUO_NET_POLLER_H