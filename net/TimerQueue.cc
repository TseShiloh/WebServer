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
                int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
                if (timerfd < 0) {// 创建失败
                    LOG_SYSFATAL <<  "Failed in timerfd_create";
                }
                return timerfd;
            }

            // 计算超时时刻与当前时间的时间差
            struct timespec howMuchTimeFromNow(Timestamp when)
            {
                int64_t microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
                if (microseconds < 100) {
                    microseconds = 100;
                }
                struct timespec ts;
                ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
                ts.tv_nsec = static_cast<long>((microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
                return ts;
            }
            
            // 清除定时器，避免一直触发
            void readTimerfd(int timerfd, Timestamp now) {
                uint64_t howmany;
                ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
                LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
                if (n != sizeof howmany) {
                    LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8"
                }
            }

            // 重置定时器的超时时间
            void resetTimerfd(int timerfd, Timestamp expiration) {
                // wake up loop by timerfd_settime()
                struct itimerspec newValue;
                struct itimerspec oldValue;
                bzero(&newValue, sizeof newValue);
                bzero(&oldValue, sizeof oldValue);
                newValue.it_value = howMuchTimeFromNow(expiration);// 将超时时刻expiration转换为itimerspec类型
                int ret = ::timerfd_settime
            }

        } // namespace detail
    } // namespace net
} // namespace muduo

using namespace muduo;
using namespace muduo::net;
using namespace muduo::net::detail;

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
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

TimerId TimerQueue::addTimer(const TimerCallback& cb,   // 定时器回调函数
                             Timestamp when,            // 超时时间
                             double interval)           // 间隔时间
{
    Timer* timer = new Timer(cb, when, interval);
    loop_->runInLoop(boost::bind(&TimerQueue::addTimerInLoop, this, timer));
    addTimerInLoop(timer);
    return TimerId(timer, timer->sequence());
}

void TimerQueue::cancel(TimerId timerId) {
    loop_->runInLoop(boost::bind(&TimerQueue::cancelInLoop, this, timerId));
    // runInLoop能保证cancel()跨线程调用
    cancelInLoop(timerId);
}

void TimerQueue::addTimerInLoop(Timer* timer) {
    loop_->assertInLoopThread();
    bool earliestChanged = insert(timer);// 这里不需要加锁
    // 插入定时器，有可能会使得最早到期的定时器发生改变

    if (earliestChanged) {// 最早到期的定时器发生了改变
        // 重置定时器的超时时刻（timerfd_settime）
        resetTimerfd();
    }
}

void TimerQueue::cancelInLoop(TimerId timerId) {
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    ActiveTimer timer(timerId.timer_, timerId.sequence_);
    // 查找该定时器
    if (it != activeTimers_.end()) {// 如果找到了
        size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
        assert(n ==1);
        (void)n;
        delete it->first; // FIXME: no delete please,如果用了unique_ptr,这里就不需要手动删除了

        activeTimers_.erase(it);
    }
    else if (callingExpiredTimers_) {
        // 已经到期，并且正在调用回调函数的定时器
        cancelingTimers_.insert(timer);
    }
    assert(timers_.size() == activeTimers_.size());
}

void TimerQueue::handleRead() {
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    readTimerfd(timerfd_, now);// 清除该事件，避免一直触发

    // 获取该时刻之前所有的定时器列表（即超时定时器列表）
    std::vector<Entry> expired = getExpired(now);

    callingExpiredTimers_ = true;// 处于“处理到期定时器”状态中
    cancelingTimers_clear();// 清空已经取消的定时器

    for (std::vector<Entry>::iterator it = expired.begin(); it != expired.end(); ++it)) {
        it->second->run();// 回调定时器处理函数，即Timer类的run()函数 -> callback_()
    }
    
    callingExpiredTimers_ = false;// 处理到期定时器”状态结束

    reset(expired, now);// 不是一次性定时器，则重启

}

// rvo优化（return value optimization），此处不会再调用拷贝构造函数从而影响性能
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now) {
    assert(timers_.size() == activeTimers_.size());
    std::vector<Entry> expired;
    Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
    // 返回第一个未到期的Timer的迭代器
    // lower_bound的含义是返回第一个“值 >= sentry”的元素的iterator
    // 即*end >= sentry，从而end->first > now
    TimeList::iterator end = timers_.lower_bound(sentry);
    assert(end == timers_.end() || end->first > now);

    // 将到期的定时器插入到expired中
    std::copy(timers_.begin(), end, back_inserter(expired));

    // 从timers_中移除到期的定时器
    timers_.erase(timers_.begin(), end);

    // 从activeTimers_中移除到期的定时器
    for (std::vector<Entry>::iterator it = expired.begin(); it != expired.end(); ++it)) {
        ActiveTimer timer(it->second, it->second->sequence());
        size_t n = activeTimers_.erase(timer);
        assert(n == 1);
        (void) n;
    }

    assert(timers_.size() == activeTimers_.size());
    return expired;
}

void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now) {
    Timestamp nextExpire;

    for (std::vector<Entry>::const_iterator it = expired.begin(); it != expired.end(); ++it)) {
        ActiveTimer timer(it->second, it->second->sequence());

        // 如果是“重复的”且“未取消的”定时器，则重启该定时器
        if (it->second->repeat() && cancelingTimers_.find(timer) == cancelingTimers_.end()) {
            it->second->restart(now);// restart的时候会重新计算下一个超时时刻
            insert(it->second);
        }
        else {
            // 一次性定时器或者已被取消的定时器是不能重置的，因此删除该定时器
            // FIXME move to a free list
            delete it->second;// FIXME: no delete please
        }
    }

    if (!timers_.empty()) {
        // 获取最早到期的定时器超时时间
        nextExpire = timers_.begin()->second->expiration();
    }

    if (nextExpire.valid()) {
        // 重置定时器的超时时刻（timerfd_settime）
        resetTimerfd(timerfd_, nextExpire);
    }
}

bool TimeQueue::insert(Timer* timer) {
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    // 最早到期时间是否改变
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    if (it == timers_.end() || when < it->first) {
    // 如果timers_为空 或者 when小于timers_中最早到期时间
        earliestChanged = true;
    }
    {
        // 插入到timers_中，<时间戳，地址>
        std::pair<TimerList::iterator, bool> result = timers_.insert(Entry(when, timer));
        assert(result.second);
        (void) result;
    }
    {
        // 插入到activeTimers_中，<地址，序号>
        std::pair<ActiveTimerSet::iterator, bool> result = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
        assert(result.second);
        (void) result;
    }
    assert(timers_.size() == activeTimers_.size());
    return earliestChanged;
}