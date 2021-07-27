#include <muduo/net/poller/PollPoller.h>

#include <muduo/base/Logging.h>
#include <muduo/base/Types.h>
#include <muduo/net/Channel.h>

#include <assert.h>
#include <poll.h>

using namespace muduo;
using namespace muduo::net;

PollPoller::PollPoller(EventLoop* loop)
    : Poller(loop)
{
}

PollPoller::~PollPoller()
{
}

Timestamp PollPoller::poll(int timeoutMs, ChannelList* activeChannels) {
    // XXX pollfds_ shouldn't change
    int numEvents = ::poll(&*pollfds_.begin(), pollfds_.size(), timeoutMs);// （数组首地址，大小，timeoutMs）
    Timestamp now(Timestamp::now());// 时间戳
    if (numEvents > 0) {// >0 说明 返回了一些事件
        LOG_TRACE << numEvents << " events happended";
        fillActiveChannels(numEvents, activeChannels);// 将事件放入“活动的通道”里面
    }
    else if (numEvents == 0) {
        LOG_TRACE << " nothing happended";
    }
    else {
        LOG_SYSERR << "PollPoller::poll()";
    }
    return now;
}


void PollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const {
    for (PollFdList::const_iterator pfd = pollfds_.begin(); 
        pfd != pollfds_.end() && numEvents > 0; ++pfd) 
    {
        if (pfd->revents > 0) {
            --numEvents;
            ChannelMap::const_iterator ch = channels_.find(pfd->fd);
            assert(ch != channels_.end());
            Channel* channel = ch->second;
            assert(channel->fd() == pfd->fd);
            channel->set_revents(pfd->revents);

            activeChannels->push_back(channel);
        }
    }
}

// 用于注册或者更新通道
void PollPoller::updateChannel(Channel* channel) {
    Poller::assertInLoopThread();
    LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();
    if (channel->index() < 0) {
        // index < 0 说明是一个新的通道
        // a new one, add to pollfds_
        assert(channels_.find(channel->fd() == channels_.end());
        struct pollfd pfd;
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        pollfds_.push_back(pfd);
        int idx = static_cast<int>(pollfds_.size()) - 1;
        channel->set_index(idx);
        channels_[pfd.fd] = channel;    
    }
    else {
        // update existing one
        assert(channels_.find(channel->fd()) != channels_.end());
        assert(channels_[channel->fd()] == channel);
        int idx = channel->index();
        assert(idx >= 0 && idx < static_cast<int>(pollfds_.size()));
        struct pollfd& pfd = pollfds_[idx];
        assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd()-1);// -channel->fd()-1见下
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        // 将一个通道暂时更改为不关注事件，但不从Poller中移除该通道
        if (channel->isNoneEvent()) {
            // ignore this pollfd
            // 暂时忽略该文件描述符的事件
            // 这里pfd.fd可以直接设置为-1
            pfd.fd = -channel->fd() - 1;// 这样子设置是为了removeChannel优化（优化“0”文件描述符）
        }
    }
}

void PollPoller::removeChannel(Channel* channel) {
    // 从通道数组中移除，不关注了
    Poller::assertInLoopThread();
    LOG_TRACE << "fd = " << channel->fd();
    assert(channels_.find(channel->fd()) != channels_.end());
    assert(channels_[channel->fd()] == channel);
    assert(ichannel->isNoneEvent());// 通道中没有事件了才能被移除
    int idx = channel->index();// 取出channel在数组中的下标位置
    assert(idx >= 0 && idx < static_cast<int>(pollfds_.size()));
    const struct pollfd& pfd = pollfds_[idx]; (void)pfd;
    assert(pfdfd == -channel->fd()-1 && pfd.events == channel->events());
    size_t n = channels_.erase(channel->fd());// 从通道中移除
    // channel是一个map，这里选择用key来移除
    assert(n == 1); (void)n;
    if (implicit_cast<size_t>(idx) == pollfds_.size() - 1) {
        pollfds_.pop_back();
    }
    else {
        //算法的时间复杂度为O(1)，将待删除元素与最后一个元素交换再pop_back
        int channelAtEnd = pollfds_.back().fd;
        iter_swap(pollfds_.begin() + idx, pollfds_.end() - 1);
        if (channelAtEnd < 0) {// 如果是暂时不关注的通道，则要还原回来
            channelAtEnd = -channelAtEnd-1;
        }
        channels_[channelAtEnd]->set_index(idx);
        pollfds_.pop_back();
    }
}
