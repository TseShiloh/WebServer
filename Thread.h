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
        static void* startThread(void* thread); // 线程的入口函数
        void runInThread();

        bool        started_;
        pthread_t   pthreadId_; // 线程在进程中的pid（局部的）
        pid_t       tid_;       // 线程真实的pid（全局的）
        ThreadFunc  func_;
        string      name_;

        static AtomicInt32 numCreated_;

    public:
        typedef boost::function<void()> ThreadFunc;

        explicit Thread(const ThreadFunc&, const string& name = string());
        ~Thread();

        void start();   // 启动线程
        int join();     // return pthread_join()

        bool started() const { return started_; }       // 线程是否已启动
        pid_t tid() const { return tid_; }              // 线程真实pid
        const string& name() const { return name_; }    // 线程的名称
        
        static int numCreated() { return numCreated_.get(); }   // 已经启动的线程个数
    };

    
}// namespace muduo

#endif
