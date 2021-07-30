#include <WebServer/net/EventLoopThread.h>

#include <WebServer/net/EventLoop.h>

#include <boost/bind.hpp>

using namespace muduo;
using namespace muduo::net;

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb)
    : loop_(NULL),
      exiting_(false),
      thread_(boost::bind(&EventLoopThread::threadFunc, this)),
      mutex_(),
      cond_(mutex_),// 条件变量和互斥量配合使用
      callback_(cb)
{

}

EventLoopThread::~EventLoopThread() {
    exiting_ = true;
    loop_->quit();//  退出IO线程，让IO线程的loop循环退出，从而退出了IO线程
    thread_.join();// 归并线程
}

EventLoopThread::EventLoop* startLoop() {// 启动线程，该线程就成为了IO线程
    asser(!thread_.started());
    thread_.start();// 线程启动了之后，会调用线程的回调函数threadFunc

    {
        MutexLockGuard lock(mutex_);
        while (loop_ == NULL) {
            cond_.wait();
        }
    }
    return loop_;
}

void EventLoopThread::threadFunc() {// 线程函数，会在线程中创建一个EventLoop对象
    EventLoop loop;

    if (callback_) {
        callback_(&loop);
    }

    {
        MutexLockGuard lock(mutex_);
        // loop_指针指向了一个栈上的对象，threadFunc函数退出之后，这个指针就失效了
        // threadFunc函数退出，就意味着线程退出了，EventLoopThread对象也就没有存在的价值了。
        // 因而不会有什么大的问题
        loop_ = &loop;
        cond_.notify();
    }

    loop.loop();
}