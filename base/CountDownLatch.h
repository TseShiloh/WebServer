/* 倒计时门闩类

    1. 用于所有子线程等待主线程发出“起跑”命令，
    意味着主线程要用Condition通知所有子线程（broadcast）

    2. 也可以用于主线程等待子线程初始化完毕才开始工作


*/

#ifndef MUDUO_BASE_COUNTDOWNLATCH_H
#define MUDUO_BASE_COUNTDOWNLATCH_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>

#include <boost/noncopyable.hpp>

namespace muduo
{
    class CountDownLatch
    {
    private:
        mutable MutexLock mutex_;// 在函数getCount()中需要改变
        Condition condition_;
        int count_;
    public:
        explicit CountDownLatch(int count);

        void wait();

        void countDown();

        int getCount() const;
    }; // class CountDownLatch
    
    
} // namespace muduo

#endif // MUDUO_BASE_COUNTDOWNLATCH_H