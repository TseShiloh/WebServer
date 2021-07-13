// 条件变量类
#ifndef MUDUO_BASE_CONDITION_H
#define MUDUO_BASE_CONDITION_H

#include <muduo/base/Mutex.h>

#include <boost/noncopyable.hpp>
#include <pthread.h>

namespace muduo
{
    class Condition : boost::noncopyable
    {
    private:
        MutexLock& mutex_;      // 此处引用，说明MutexLockGuard这个类并不负责MutexLock类对象mutex_的生存期
        pthread_cond_t pcond_;  // 条件变量对象

    public:
        explicit Condition (MutexLock& mutex_)
            : mutex_(mutex)
        {
            pthread_cond_init(&pcond_, NULL);
        }

        ~Condition() {
            pthread_cond_destory(&pcond_);
        }

        void wait() {
            pthread_cond_wait(&pcond_, mutex_.getPthreadMutex());
        }

        // returns true if time out, false otherwise.
        bool waitForSeconds(int seconds);

        void notify() {
            pthread_cond_signal(&pcond_);
        }

        void notifyall() {
            pthread_cond_signal(&pcond_);
        }
    }; // class Condition
    
}// namespace muduo

#endif // MUDUO_BASE_CONDITION_H

/* 条件变量一个很关键的地方
    [线程1]
    锁住 mutex
        while (条件)
            等待wait();
    解锁 mutex

    [线程2]
    锁住 mutex
        更改条件
            signal或broadcast
    解锁 mutex

线程1的wait()函数做了三件事情：解锁->等待条件变量->加锁->返回
*/