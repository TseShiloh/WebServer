#include <WebServer/net/Timer.h>

using namespace muduo;
using namespace muduo::net;

AtomicInt64 Timer::s_numCreated_;

void Timer::restart(Timestamp now) {
    if (repeat_) { // 如果是重复的定时器
        // 重新计算下一个超时时刻now+interval_
        expiration_ = addTime(now, interval_);
    }
    else {// 不是重复定时器
        // 下一个超时时刻设置为一个非法时间
        expiration_ = Timestamp::invalid();
    }
}


/*
addTime()是一个全局函数，在Timestamp中定义。
其中inline Timestamp addTime(Timestamp timestamp, double seconds)用的是值传递。
因为Timestamp类只有一个数据成员int64_t microSecondsSinceEpoch_，
因此可以将timestamp看成一个64位的整数，
所以参数传递到8字节的寄存器当中，而不是堆栈，反而能提高效率
*/