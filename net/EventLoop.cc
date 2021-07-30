#include <muduo/net/EventLooping.h>
#include <muduo/base/Logging.h>

#include <poll.h>

using namespace muduo;
using namespace muduo::net;

namespace
{
    // 当前线程EventLoop对象指针（线程局部存储--非全局变量）
    __thread EventLoop* t_loopInThisThread = 0;

    const int kPollTimeMs = 10000;// 10s

    int createEventfd() {
        int evtfd = ::eventfd(0, )
    }
}

EventLoop* EventLoop::getEventLoopOfCurrentThread() {
    return t_loopInThisThread;
}

EventLoop::EventLoop()
    : looping_(false),// 初始化为false表示当前还没有处于循环的状态
      quit_(false),
      eventHandling_(false),
      threadId_(CurrentThread::tid())// 把当前线程的真实id初始化给threadId_
      poller_(Poller::newDefaultPoller(this)),
      timerQueue_(new TimerQueue(this)),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_)),
      currentActiveChannel_(NULL)
{
    LOG_TRACE << "EventLoop created " << this << " in thread " << threadId_;
    
    // 每个线程最多有一个EventLoop对象
    if (t_loopInThisThread) {   // 当前线程已经创建了EventLoop对象，终止
        LOG_FATAL << "Another EventLoop " << t_loopInThisThread
                  << " exists in this thread" << threadId_;
    }
    else {
        t_loopInThisThread = this;
    }
    wakeupChannel_->setReadCallback(boost::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableReading();// 纳入到poller来管理
}

EventLoop::~EventLoop() {
    t_loopInThisThread = NULL;
}

void EventLoop::loop() {    // 事件循环，该函数不能跨线程调用，只能在创建该对象的线程中调用
    assert(!looping_);      // 断言还没有循环
    assertInLoopThread();   // 断言当前处于“创建该线程”的对象当中
    looping_ = true;
    LOG_TRACE << "EventLoop " << this << " start looping";

    //::poll(NULL, 0, 5*1000);
    while (!quit_) {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        // ++iteration_;
        if (Logger::logLevel() <= Logger::TRACE) {
            printActiveChannels();
        }
        // TODO sort channel by priority
        eventHandling_ = true;
        for (ChannelList::iterator it = activeChannels_.begin();
            it != activeChannels_.end(); ++it)
        {
            currentActiveChannel_ = *it;
            currentActiveChannel_->handleEvent(pollReturnTime_);
        }
        currentActiveChannel_ = NULL;// 处理完后
        eventHandling_ = false;
        doPendingFunctors();// 没有反复执行到pendingFunctors为空
    }

    LOG_TRACE << "EventLoop " << this << " stop looping";
    looping_ = false;
}

// 该函数可以跨线程调用
void EventLoop::quit() {
    // quit_是bool型，在Linux底下bool型是原子性操作，不需要原子性保护
    quit_ = true;
    if (!isInLoopThread()) {    // 如果不是当前线程调用，则还需唤醒
        wakeup();
        // 更好的方法：eventfd
    }
}

// 在I/O线程中执行某个回调函数，该函数可以跨线程调用
void EventLoop::runInLoop(const Functor& cb) {
    if (isLoopThread()) {   
        // 如果是当前IO线程调用runInLoop，则同步调用cb
        cb();
    }
    else {                  
        // 如果是其它线程调用runInLoop，则异步地将cb添加到队列
        queueInLoop(cb);
    }
}

// 将回调任务添加到队列当中
void EventLoop::queueInLoop(const Functor& cb) {
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.push_back(cb);
    }
    
    // 调用queueInLoop的线程不是当前IO线程，则需要唤醒
    // 或者调用queueInLoop的线程是当前IO线程，并且此时正在调用pending functor，需要唤醒？？
    // 只有当前IO线程的事件回调中调用queueInLoop才不需要唤醒
    if (!isInLoopThread() || callingPendingFunctors_) {
        wakeup();
    }
}

TimerId EventLoop::runAt(const Timestamp& time, const TimerCallback& cb) {
  return timerQueue_->addTimer(cb, time, 0.0);// (，到期时间，非重复)
}

TimerId EventLoop::runAfter(double delay, const TimerCallback& cb) {
  Timestamp time(addTime(Timestamp::now(), delay));
  return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback& cb) {
  Timestamp time(addTime(Timestamp::now(), interval));
  return timerQueue_->addTimer(cb, time, interval);
}

void EventLoop::cancel(TimerId timerId) {
  return timerQueue_->cancel(timerId);
}

void EventLoop::updateChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);// 操作channel的应当是所属的本对象
    assertInLoopThread();// 在EventLoop线程当中
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    if (eventHandling_) {
        assert(currentActiveChannel_ == channel || 
               std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end())
    }
    poller_->removeChannel(channel);
}

void EventLoop::abordNotInLoopThread() {
    LOG_FATAL << "EventLoop::abordNotInLoopThread - EventLoop " << this
              << " was created in threadId_ = " << threadId_
              << ", current thread id = " << CurrentThread::tid();
}

void EventLoop::wakeup() {// 一个线程可以唤醒另一个线程
    uint64_t one = 1;// 8个字节的缓冲区
    ssize_t n = sockets::write(wakeupfd_, &one, sizeof one);// 写入8个字节
    if (n != sizeof one) {
        LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::handleRead() {
    unit64_t one = 1;
    ssize_t n = sockets::read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;     // 处于“调用回调任务ing”的状态中
    {
        MutexLockGuard lock(mutex_);    // 保护临界区（只保护这两行内的临界区）
        functors.swap(pendingFunctors_);// 把回调列表swap到functors中
        // 这样做一方面减小了临界区的长度，意味着不会阻塞其他线程的queueInLoop()
        // 另一方面，避免了死锁，因为Functor可能再次调用queueInLoop()
    }

    for (size_t i = 0; i < functors.size(); ++i) {
        functors[i]();
    }
    callingPendingFunctors_ = false;    // 结束“调用回调任务”的状态
}
// 没有反复执行到pendingFunctors为空，这是有意的，否则IO线程可能陷入死循环，无法处理IO事件。

void EventLoop::printActiveChannels() const {
    for (ChannelList::const_iterator it = activeChannels_.begin();
         it != activeChannels_.end(); ++it)
    {
        const Channel* ch = *it;
        LOG_TRACE << "{" << ch->reventsToString() << "}";
    }
}