// 倒计时门闩类
#include <muduo/base/CountDownLatch.h>

using namespace muduo;

CountDownLatch::CountDownLatch(int count)
    :   mutex_(),
        condition_(mutex_),// 只是个引用，不负责mutex_的生存期
        count_(count)
{
}
    
void CountDownLatch::wait() {
    MutexLockGuard lock(mutex_);
    while (count_ > 0) {
        condition_.wait();
    }
}

void CountDownLatch::countDown() {
    MutexLockGuard lock(mutex_);
    --count;
    if (count_ == 0) {
        condition_.notifyall();
    }
}

int CountDownLatch::getCount() const {
    MutexLockGuard lock(mutex_);
    return count_;
}