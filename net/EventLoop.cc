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
      currentActiveChannel_(NULL)
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
        // doPendingFunctors
    }

    LOG_TRACE << "EventLoop " << this << " stop looping";
    looping_ = false;
}

// 该函数可以跨线程调用
void EventLoop::quit() {
    // quit_是bool型，在Linux底下bool型是原子性操作，不需要原子性保护
    quit_ = true;
    if (!isInLoopThread()) {// 如果不是当前线程调用，则还需唤醒
        //wakeup();
        // 更好的方法：eventfd
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

void Event::updateChannel(Channel* channel) {
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

void EventLoop::printActiveChannels() const {
    for (ChannelList::const_iterator it = activeChannels_.begin();
        it != activeChannels_.end(); ++it)
    {
        const Channel* ch = *it;
        LOG_TRACE << "{" << ch->reventsToString() << "}";
    }
}