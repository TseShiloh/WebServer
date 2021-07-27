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
    size
    semFull(size)
    semEmpty(0)
    mutex互斥量

    生产者                       消费者
    while(queue.full())无界则删  while(queue.empty())无界则删
    { notFull.wait(); }         { notEmpty.wait(); }

    lock(mutex)                 lock(mutex)
    queue.push();               x = queue.pop();
    unlock(mutex)               unlock(mutex)
    notEmpty.signal()           notFull.signal()无界则删
*/

#ifndef MUDUO_BASE_BOUNDEDBLOCKINGQUEUE_H
#define MUDUO_BASE_BOUNDEDBLOCKINGQUEUE_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>

#include <boost/circular_buffer.hpp>
#include <boost/noncopyable.hpp>
#include <assert.h>

namespace muduo
{
    template<typename T>
    class BoundedBlockingQueue : boost ::noncopyable
    {
    private:
        mutable MutexLock           mutex_;
        Condition                   notEmpty_;
        Condition                   notFull_;
        boost::circular_buffer<T>   queue_;// 环形缓冲区

    public:
        explicit BoundedBlockingQueue(int maxSize)
            : mutex_(),
              notEmpty_(mutex_),
              notFull_(mutex_),
              queue_(maxSize)
        {}

        void put(const T& x);
        T take();

        bool empty() const;
        bool full() const;

        size_t size() const;
        size_t capacity() const;
    }; // class BoundedBlockingQueue


    void BoundedBlockingQueue::put(const T& x) {
        {
            MutexLockGuard lock(mutex_);
            while (queue_.full()) {
                notEmpty_.wait();
            }
            assert(!queue_.full());
            qeue_.push_back(x);
        }
        notEmpty_.notify(); // TODO: move outside of lock
    }

    T BoundedBlockingQueue::take() {
        {
            MutexLockGuard lock(mutex_);
            while (queue_.empty()) {
                notEmpty_.wait();
            }
            assert(!queue_.empty());
            T front(queue_.front());
            queue_pop_front();
        }
        notFull_.notify(); // TODO: move outside of lock
        return front;
    }

    bool BoundedBlockingQueue::empty() const {
        MutexLockGuard lock(mutex_);
        return queue_.empty();
    }

    bool BoundedBlockingQueue::full() const {
        MutexLockGuard lock(mutex_);
        return queue_.full();

    }

    size_t BoundedBlockingQueue::size() const{
        MutexLockGuard lock(mutex_);
        return queue_.size();
    }

    size_t BoundedBlockingQueue::capacity() const {
        MutexLockGuard lock(mutex_);
        return queue_.capacity();
    }


} // namespace muduo

#endif // MUDUO_BASE_BOUNDEDBLOCKINGQUEUE_H

// 可自行实现一下环形缓冲区