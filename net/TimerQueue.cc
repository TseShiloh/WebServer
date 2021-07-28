#define __STDC_LIMIT_MACROS
#include <WebServer/net/TimerQueue.h>

#include <WebServer/base/Logging.h>
#include <WebServer/net/EventLoop.h>
#include <WebServer/net/Timer.h>
#include <WebServer/net/TimerId.h>

#include <boost/bind.hpp>

#include <sys/timerfd.h>

namespace muduo
{
    namespace net
    {
        namespace detail
        {
            // 创建定时器
            int createTimerfd() {

            }
        } // namespace detail
    } // namespace net
} // namespace muduo

using namespace muduo;
using namespace muduo::net;
using namespace muduo::net::detail;

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
      timerfd_(),
      timerfdChannel_(),
      timers_(),
      callingExpiredTimers_()
{
    timerfdChannel_.setReadCallback(boost::bind(&TimerQueue::handleRead, this));
    timerfdChannel_.enableReading();// 将由Poller的updateChannel()关注
}

TimerQueue::~TimerQueue() {
    ::close(timerfd_);
    // Do not remove channel, since we're in EventLoop::dtor()
    for (TimerList::iterator it = timers_.begin(); it != timers_.end(); ++it)
    {
        delete it->second;
    }
}

TimerId TimerQueue::addTimer(const TimerCallback& cb,
                             Timestamp when,
                             double interval)
{

}

void TimerQueue::cancel(TimerId timerld) {
    //
    cancelInLoop(timerId);
}

void TimerQueue::addTimerInLoop(Timer* timer) {
    loop->assertInLoopThread();
    bool earliestChanged = insert(timer);// 这里不需要加锁

    if (earliestChanged) {
        // 重置定时器的超时时刻（timerfd——settime）
        resetTimerfd();
    }
}

void TimerQueue::cancelInLoop(TimerId timerId) {
    loop->assertInLoopThread();


}

void TimerQueue::handleRead() {
    loop->assertInLoopThread();
    Timestamp now(Timestamp::now());


}