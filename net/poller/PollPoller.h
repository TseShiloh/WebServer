/*
PollPoller类是Poller的派生类

PollPoller类是对Poll的封装
*/

#ifndef MUDUO_NET_POLLER_POLLPOLLER_H
#define MUDUO_NET_POLLER_POLLPOLLER_H

#include <muduo/net/Poller.h>
#include <map>
#include <vector>

struct pollfd;

namespace muduo
{
    namespace net
    {
        class PollPoller : public Poller
        {
        private:
            void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
            
            typedef std::vector<struct pollfd> PollFdList;// 是一个向量，一个数组
            PollFdList pollfds_;
            
            typedef std::map<int, Channel*> ChannelMap;// key是文件描述符，value是Channel*
            ChannelMap channels_;// 所关注的通道列表

        public:
            PollPoller(EventLoop* loop);
            virtual ~PollPoller();

            virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels);// poll的时候会返回“活动的通道列表”
            virtual void updateChannel(Channel* channel);
            virtual void removeChannel(Channel* channel);

        }; // class PollPoller

    } // namespace net

} // namespace muduo


#endif // MUDUO_NET_POLLER_POLLPOLLER_H