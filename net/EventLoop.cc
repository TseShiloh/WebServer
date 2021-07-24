#include <muduo/net/EventLooping.h>
#include <muduo/base/Logging.h>

#include <poll.h>

using namespace muduo;
using namespace muduo::net;

namespace
{
    // 当前线程EventLoop对象指针（线程局部存储--非全局变量）
    __thread EventLoop* t_loopInThisThread = 0;
}

EventLoop* EventLoop::getEventLoopOfCurrentThread() {
    return t_loopInThisThread;
}

EventLoop::EventLoop()
    : looping_(false),// 初始化为false表示当前还没有处于循环的状态
      threadId_(CurrentThread::tid())// 把当前线程的真实id初始化给threadId_
{
    LOG_TRACE << "EventLoop created " << this << " in thread " << threadId_;
    
    // 每个线程最多有一个EventLoop对象
    if (t_loopInThisThread) {   // 如果已经创建，终止程序
        LOG_FATAL << "Another EventLoop " << t_loopInThisThread
                  << " exists in this thread" << threadId_;
    }
    else {
        t_loopInThisThread = this;
    }
}

EventLoop::~EventLoop() {
    t_loopInThisThread = NULL;
}

void EventLoop::loop() {    // 事件循环，该函数不能跨线程调用，只能在创建该对象的线程中调用
    assert(!looping_);      // 断言还没有循环
    assertInLoopThread();   // 断言当前处于“创建该线程”的对象当中
    looping_ = true;
    LOG_TRACE << "EventLoop " << this << " start looping";

    ::poll(NULL, 0, 5*1000);

    LOG_TRACE << "EventLoop " << this << " stop looping";
    looping_ = false;
}

void EventLoop::abordNotInLoopThread() {
    LOG_FATAL << "EventLoop::abordNotInLoopThread - EventLoop " << this
              << " was created in threadId_ = " << threadId_
              << ", current thread id = " << CurrentThread::tid();
}