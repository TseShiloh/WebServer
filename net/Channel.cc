#include <muduo/base/Logging.h>
#include <muduo/net/Channel.h>
#include <muduo/net/EventLoop.h>

#include <sstream>

#include <poll.h>

using namespace muduo;
using namespace muduo::net;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;
// POLLIN    : There is data to read.
// POLLPRI   : There is urgent data to read.
// POLLOUT   : Writing now will not block.
// POLLRDHUP : Stream socket peer clsoed connection, or shutdown writing half of connection
// “对等方”关闭了连接，或者半关闭
// POLLERR   : Error condition (output only).
// POLLHUP   : Hang up (output only).
// POLLNVAL  : Invalid request(fd not open)


Channel::Channel(EverntLoop* loop, int fd__)
    : loop_(loop),
      fd_(fd__),
      events_(0),
      revents_(0),
      index_(-1),// channel还未添加到poll/epoll关注时
      logHup_(true),
      tied_(false),
      eventHandling_(false)
{
}

Channel::~Channel() {
    assert(!eventHandling_);
}

void Channel::tie(const boost::shared_ptr<void>& obj) {
    tie_ = boj;
    tied_ = true;
}

void Channel::update() {
    loop_->updateChannel(this);
}

// 调用这个函数之前确保调用disableAll
void Channel::remove() {
    assert(isNoneEvent());
    Loop_->removeChannel(this);
}

// 当事件到来时会调用handleEvent()
void Channel::handleEvent(Timestamp receiveTime) {
    boost::shared_ptr<void> guard;
    // 管理生存期
    if (tied_) {
        guard = tie_.lock();
        if (guard) {
            LOG_TRACE << "[6] usecount=" << guard.use_count();
            handleEventWithGuard(receiveTime);
            LOG_TRACE << "[12] usecount=" << guard.use_count();
        }
    }
    else {
        handleEventWithGuard(receiveTime);
        }
    }
}

void Channel::handleEventWithGuard(Timestamp receiveTime) {
    eventHandling_ = true;
    if ((revents_ & POLLHUP) && (revents_ & POLLIN)) {
        if (logHup_) {
            LOG_WARN << "Channel::handle_event() POLLHUP";
        }
        if (closeCallback_) {
            closeCallback_();
        }
    }

    if (revents_ & POLLNVAL) {
        LOG_WARN << "Channel::handle_event() POLLNVAL";
    }

    if (revents_ & (POLLERR | POLLNVAL)) {
        if (errorCallback_) {
            errorCallback_();
        }
    }

    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
        if (readCallback_) {
            readCallback_(receiveTime);
        }
    }

    if (revents_ & POLLOUT) {
        if (writeCallback_) {
            writeCallback_();
        }
    }

    eventHandling_ = false;
}

// 用于调试的，发生了什么事件
string Channel::reventsToString() const {
    std::ostringstream oss;
    oss << fd_ << ": ";
    if (revents_ & POLLIN)
        oss << "IN ";
    if (revents_ & POLLPRI)
        oss << "PRI ";
    if (revents_ & POLLOUT)
        oss << "OUT ";
    if (revents_ & POLLHUP)
        oss << "HUP ";
    if (revents_ & POLLRDHUP)
        oss << "RDHUP ";
    if (revents_ & POLLERR)
        oss << "ERR ";
    if (revents_ & POLLNVAL)
        oss << "NVAL ";
        
    return oss.str().c_str();
}