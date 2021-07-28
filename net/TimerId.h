#ifndef MUDUO_NET_TIMERID_H
#define MUDUO_NET_TIMERID_H

#include <WebServer/base/copyable.h>

namespace muduo
{
    namespace net
    {
        class Timer;

        ///
        /// An opaque identifier, for canceling Timer(即外部可见的)
        ///
        class TimerId : public muduo::copyable
        {
        private:
            Timer*   timer_;        // 定时器地址，Timer里面也包含了定时器序号
            int64_t  sequence_;     // 定时器序号
        public:
            friend class TimerQueue;

            TimerId()
                : timer_(NULL),
                  sequence_(0)
            {}

            TimerId(Timer* timer, int64_t seq)
                : timer_(timer),
                  sequence_(seq)
            {}

        }; // class TimerId

    } // namespace net
} // namespace muduo

#endif // MUDUO_NET_TIMERID_H