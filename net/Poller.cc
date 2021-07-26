/*
Poller和EventLoop是组合关系
Poller对象的生存期由EventLoop控制
EventLoop的loop()函数通过调用Poller的poll()实现
*/

#include <muduo/net/Poller.h>

using namespace muduo;
using namespace muduo::net;

Poller::Poller(EventLoop* loop)
    : ownerLoop_(loop)
{}

Poller::~Poller()
{}