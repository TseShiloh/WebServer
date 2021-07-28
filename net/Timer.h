/*
muduo的定时器由三个类实现，Timerld, Timer, TimerQueue
用户只能看到第一个类Timerld，其他两个都是内部实现细节

Timer对定时操作进行了抽象，并没有调用任何定时器相关函数

TimerQueue的接口很简单，只有两个函数addTimer和cancel

实际使用中也并没有直接调用addTimer和cancel
而是通过EventLoop的
    runAT       在某个时刻运行定时器
    runAfter    过一段时间运行定时器
    runEvery    每隔一段时间运行定时器
    cancel      取消定时器

*/

#ifndef MUDUO_NET_TIMER_H
#define MUDUO_NET_TIMER_H

#include <boost/noncopyable.hpp>

#include <muduo/base/Atomic.h>
#include <muduo/base/Timestamp.h>
#include <muduo/net/Callbacks.h>

namespace muduo
{
    namespace net
    {
        ///
        /// Interval class for timer event.
        ///
        class Timer : boost::noncopyable
        {
        private:
            const TimerCallback   callback_;      // 定时器回调函数
            Timestamp             expiration_;    // 下一次的超时时刻
            const double          interval_;      // 超时时间间隔，一次性定时器该值为0
            const bool            repeat_;        // 是都重复
            const in64_t          sequence_;      // 定时器序号

            static AtomicInt64    s_numCreated_;  // 定时器计数，当前已经创建的定时器数量（是一个原子操作类对象）
        public:
            Timer(const TimerCallback& cb, Timestamp when, double interval)
                : callback_(cb),
                  expiration_(when),
                  interval_(interval),
                  repeat_(interval > 0.0),
                  sequence_(s_numCreated_.incrementAndGet())// 原子性操作能保证sequence_是唯一的
            {
            }

            void run() const {// 调用回调函数
                callback();
            }

            Timestamp expiration() const { return expiration_; }
            bool repeat() const { return repeat_; }
            int64_t sequence() const { return sequence_; }

            void restart(Timestamp now);

            static int64_t nowCreated() { return s_numCreated_.get(); }
            
        }; // class Timer

    } // namespace net

} // namespace muduo

#endif // MUDUO_NET_TIMER_H