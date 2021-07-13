#ifndef MUDUO_BASE_CURRENTTHREAD_H
#define MUDUO_BASE_CURRENTTHREAD_H

#include <muduo/base/Atomic.h>
#include <muduo/base/Types.h>

#include <boost/funciton.hpp>
#include <boost/noncopyable.hpp>
#include <pthread.h>

namespace muduo
{
    class Thread : boost::noncopyable
    {
    private:
        static void* startThread(void* thread);
        void runInThread();

        bool        started_;
        pthread_t   pthreadId_;// 线程在进程中的pid（局部的）
        pid_t       tid_;// 线程真实的pid（全局的）
        ThreadFunc  func_;
        string      name_;

        static AtomicInt32 numCreated_;

    public:
        typedef boost::function<void()> ThreadFunc;

        explicit Thread(const ThreadFunc&, const string& name = string());
        ~Thread();

        void start();
        int join();// return pthread_join()

        bool started() const { return started_; }
    };

    
}// namespace muduo











#endif
