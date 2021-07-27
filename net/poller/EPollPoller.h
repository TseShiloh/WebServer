/*
PollPoller类是Poller的派生类

PollPoller类是对Poll的封装
*/

#ifndef MUDUO_NET_POLLER_EPOLLPOLLER_H
#define MUDUO_NET_POLLER_EPOLLPOLLER_H

#include <muduo/net/Poller.h>
#include <map>
#include <vector>

struct epoll_event;

namespace muduo
{
    namespace net
    {
        class EPollPoller : public Poller
        {
        private:
            static const int kInitEventListSize = 16;// 初始分配事件个数
            
            void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
            void update(int operation, Channel* channel);

            typedef std::vector<struct epoll_event> EventList;
            typedef std::map<int, Channel*>         ChannelMap;// key是文件描述符fd，value是Channel*
            
            int         epollfd_;
            EventList   events_;
            ChannelMap  channels_;  // 所关注的通道列表

        public:
            EPollPoller(EventLoop* loop);
            virtual ~EPollPoller();

            virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels);// poll的时候会返回“活动的通道列表”
            virtual void updateChannel(Channel* channel);
            virtual void removeChannel(Channel* channel);

        }; // class PollPoller

    } // namespace net

} // namespace muduo


#endif // MUDUO_NET_POLLER_EPOLLPOLLER_H