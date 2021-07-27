/*
muduo库内部实现的epoll是LT模式，而不是ET模式
*/

#include <muduo/net/Channel.h>
#include <muduo/net/EventLoop.h>

#include <boost/bind.hpp>

#include <stdio.h>
#include <sys/timerfd.h>

using namespace muduo;
using namespace muduo::net;

EventLoop* g_loop;
int timerfd;

void timeout(Timestamp receiveTime) {
    printf("Timeout!\n");
    unit64_t howmany;
    ::read(timerfd, &howmany, sizeof howmany);// 将数据读走，否则会一直触发（处于电平触发状态）
    g_loop->quit();
}

int main(void)
{
    EventLoop loop;
    g_loop = &loop;

    timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    Channel channel(&loop, timerfd);
    channel.setReadCallback(boost::bind(timeout, _1));// 将参数传递到了timeout当中
    channel.enableReading();// 关注了（定时器）可读事件
    // enableReading会触发Channel::update()操作 -> loop_->updateChannel(this) -> poller_->updateChannel(channel)

    struct itimerspec howlong;// 设定定时器的超时时间
    bzero(&howlong, sizeof howlong);// 定时器清零
    howlong.it_value.tv_sec = 1;
    ::timerfd_settime(timerfd, 0, &howlong, NULL);

    loop.loop();// 处于事件循环的状态，等待下一个事件

    ::close(timerfd);
}