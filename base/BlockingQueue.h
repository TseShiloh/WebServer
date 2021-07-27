/* 有界缓冲区（信号量）
    size
    semFull(size)
    semEmpty(0)
    mutex互斥量

    生产者                       消费者
    p(semFull)无界则删           p(semEmpty)
    lock(mutex)                 lock(mutex)
    queue.push();               x = queue.pop();
    unlock(mutex)               unlock(mutex)
    v(semEmpty)                 p(semFull)无界则删
*/

/* 有界缓冲区（条件变量）

    生产者                       消费者
    while(queue.full())无界则删  while(queue.empty())无界则删
    { notFull.wait(); }         { notEmpty.wait(); }

    lock(mutex)                 lock(mutex)
    queue.push();               x = queue.pop();
    unlock(mutex)               unlock(mutex)
    notEmpty.signal()           notFull.signal()无界则删
*/


#ifndef MUDUO_BASE_BLOCKINGQUEUE_H
#define MUDUO_BASE_BLOCKINGQUEUE_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>

#include <boost/noncopyable.hpp>
#include <deque>
#include <assert.h>

namespace muduo 
{
    template<typename T>
    class BlockingQueue : boost::noncopyable
    {
    private:
        mutable MutexLock mutex_;
        Condition         notEmpty_;
        std::deque<T>     queue_;

    public:
        BlockingQueue()
            : mutex_(),
              notEmpty_(mutex_),
              queue_()
        {}

        void put(const T& x) {  // 生产产品
            {
                MutexLockGuard lock(mutex_);// 不负责对象mutex_的生存期
                queue_.push_back(x);
            }
            notEmpty_.notify();// TODO: move outside of lock
        }
        
        T take() {              //消费产品
            MutexLockGuard lock(mutex_);
            // always use a while-loop, due to spurious wakeup
            while(queue_.empty()) {
                notEmpty_wait();
            }
            assert(!queue_.empty());
            T front(queue_.front());
            queue_.pop_front();
            return front;
        }

        size_t size() const {// 需要保护一下，可能多个线程都访问这个成员函数
            MutexLockGuard lock(mutex_);
            return queue_.size();
        }
    }; // class BlockingQueue
} // namespace muduo


#endif // MUDUO_BASE_BLOCKINGQUEUE_H