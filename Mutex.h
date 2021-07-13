#ifndef MUDUO_BASE_MUTEX_H
#define MUDUO_BASE_MUTEX_H

#include <muduo/base.CurrentThread.h>
#include <boost/noncopyable.hpp>
#include <assert.h>
#include <pthread.h>

namespace muduo
{
    class MutexLock : noncopyable
    {
    private:
        pthread_mutex_t mutex_; // 锁对象
        pid_t holder_;          // 当前拥有该锁的线程tid（真实pid）

    public:
        MutexLock() 
            : holder_(0) // 表示这个锁不被任何一个线程拥有
        {
            int ret = pthread_mutex_init(&mutex_, NULL);// 初始化:锁mutex_，holder_=0
            assert(ret == 0);
            (void) ret;
        }

        ~MutexLock() {
            assert(holder_ == 0);// 断言这个锁没有被任何一个线程拥有
            int ret = pthread_mutex_destory(&mutex_);// 才嫩销毁这个锁
            assert(ret == 0);
            (void) ret;
        }

        //当前线程是否拥有该锁
        bool isLockedByThisThread() {
            return holder_ == CurrentThread::tid();
        }

        // 断言当前线程拥有该锁
        void assertLocked() {
            assert(isLockedByThisThread());
        }

        // 加锁
        void lock() {
            pthread_mutex_lock(&mutex_);
            holder_ = CurrentThread::tid();
        }

        // 解锁
        void unlock() {
            holder_ = 0;
            pthread_mutex_unlock(&mutex_);
        }

        // 获取锁对象
        pthread_mutex_t* gerPthreadMutex() { /* non const ? */
            return &mutex_;
        }
        

    }; // class MutexLock


    class MutexLockGuard : boost::noncopyable
    {
    private:
       MutexLock&  mutex_;// 此处引用，说明MutexLockGuard这个类并不负责MutexLock类对象mutex_的生存期

    public:
        explicit MutexLockGuard(MutexLock& mutex)
            : mutex_(mutex)
        {
            mutex_.lock();
        }

        ~MutexLockGuard() {
            mutex_.unlock();
        }
        

    }; // claee MutexLockGuard
    // 该类采用RAII技法封装，自由获取及初始化
    // 该类和MutexLock关联，可以避免判断语句return后无法解锁的问题

} // namespace muduo

// 定义下面的宏，防止错误用法：MutexLockGuard(mutex_);（构造一个匿名的MutexLockGuard对象）
// 因为一个临时对象不允许长时间拥有锁
#define MutexLockGuard(x) error "Missing guard object name"

#endif // MUDUO_BASE_MUTEX_H