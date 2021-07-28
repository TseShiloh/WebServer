/*
TimerQueue数据库的选择，能快速根据当前时间找到已到期的定时器，
也要高效的添加和删除Timer，因为可以用二叉搜索树，用map和set
    typedef std::pair<Timestamp, Timer*> Entry;
    typedef std::set<Entry> TimerList;

TimerQueue管理很多定时器，有可能时间戳是一样的，但是map的key值不能重复，所以不用map<Timestamp, Timer*>
*/

#ifndef MUDUO_NET_TIMERQUEUE_H
#define MUDUO_NET_TIMERQUEUE_H

#include <boost/noncopyable.hpp>

#include <WebServer/base/Mutex.h>
#include <WebServer/base/Timestamp.h>
#include <WebServer/net/Callbacks.h>
#include <WebServer/net/Channel.h>

#include <set>
#include <vector>

namespace muduo
{
    namespace net
    {
        class EventLoop;
        class Timer;
        class TimeId;
        
        ///
        ///
        ///
        class TimerQueue : boost::noncopyable
        {
        public:
            TimerQueue(EventLoop* loop);
            ~TimerQueue();

            /// schedules the callback to be run at given time.
            /// repeats if @c interval > 0.0
            ///
            /// Must be thread safe. Usually be called from other threads.
            TimeId addTimer(const TimerCallback& cb,
                            Timestamp when,
                            double interval);

            void cancel(TimerId timerid);

        private:
            // FIXME: use unique_ptr<Timer> instead of raw pointers.
            typedef std::pair<Timestamp, Timer*>    Entry;// 包装了<时间戳, 地址>
            typedef std::set<Entry>                 TimerList;// 这样就可以按照时间戳Timestamp来排序
            typedef std::pair<Timer*, int64_t>      ActiveTimer;// 包装了<地址, 序号>
            typedef std::set<ActiveTimer>           ActiveTimerSet;// 按照地址Timer*排序，内容其实和TimerList是一样的

            // 以下成员函数只可能在其所属的I/O线程中调用，因而不必加锁。
            // 锁竞争是服务器性能杀手之一，所以要尽可能少用锁
            void addTimerInLoop(Timer* timer);
            void cancelInLoop(TimerId timerId);
            
            void handleRead();

            std::vector<Entry> getExpired(Timestamp now);// 返回超时的定时器列表
            void reset(const std::vector<Entry>& expired, Timestamp now);// 重置超时的定时器

            bool insert(Timer* timer);

            EventLoop* loop_;// 所属EventLoop
            const int timerfd_;// 定时器文件描述符
            Channel timerfdChannel_;// 定时器通道，当定时器事件到来时，可读事件产生，会回调handleRead函数

            TimerList timers_; // timers_是按到期时间排序的定时器列表

            // for cancel()
            // timers_与activeTimers_保存的是相同的数据
            // timers_是按照到期时间排序，activeTimers_是按对象地址排序
            ActiveTimerSet activeTimers_;
            bool callingExpiredTimers_;// atomic
            // 是否处于调用“处理超时定时器”的过程中
            ActiveTimerSet cancelingTimers_;// 保存的是被取消的定时器

        }; // class TimerQueue
        
    } // namespace net
} // namespace muduo

#endif  // MUDUO_NET_TIMERQUEUE_H