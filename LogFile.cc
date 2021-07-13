#include <muduo/base/LogFile.h>
#include <muduo/base/logging.h>
#include <muduo/base/ProcessInfo.h>

#include <assert.h>
#include <stdio.h>
#include <time.h>

using namespace moduo;

// not thread safe
class LogFile::File : boost::noncopyable
{
public:
    explicit File(const string& filename) : fp_(::fopen(filename.data(), "ae")), writtenBytes_(0) {
        assert(fp_);

        ::setbuffer(fp_, buffer_, sizeof buffer_);
    }

    ~File() {
        ::fclose(fp_);          // 关闭这个文件指针
    }

    void append(const char* logline, const size_t len) {
        size_t n = write(logline, len);     // 不上锁的方式写入，非线程安全
        size_t remain = len - n;            // 剩余的字节数remain
        while (remain > 0) {                // remain>0表示没写完，需要继续写直到写完
            size_t x = write(logline + n, remain);
            if (x == 0) {
                int err = ferror(fp_);
                if (err) {
                    fprintf(stderr, "LogFile::File::append() failed %s\n", strerror_tl(err));
                }
                break;
            }
            n += x;
            remain -= x;
        }

        writtenBytes += len;
    }

    void flush() {
        ::fflush(fp_);
    }

    size_t writtenBytes() const { return writtenBytes_; }

private:

    size_t write(const char* logline, size_t len) {
#undef fwrite_unlocked
        return ::fwrite_unlocked(logline, 1, len, fp_);
    }

    FILE* fp_;                  // 文件指针fp_
    char buffer_[64*1024];      // 文件指针fp_的缓冲区buffer_，缓冲区如果超过了64k就会自动flush到文件中
    size_t writtenBytes_;       // 已经写入的字节数
};

LogFile::LogFile(const string& basename, size_t rollSize, bool threadSafe, int flushInterval)
                : basename_(basename), 
                  rollSize_(rollSize), 
                  flushInterval_(flushInterval), 
                  count_(0),
                  mutex_(threadSafe ? new MutexLock : NULL),
                  startOfPeriod_(0),
                  lastRoll_(0),
                  lastFlush_(0) {
    assert(basename.find('/') == string::npos);// 断言basename是不包含/的
    rollFile();
}

LogFile::~Logfile() {

}

void LogFile::append(const char* logline, int len) {
    if (mutex_) {       // 线程安全时
        MutexLockGuard lock(*mutex_);
        append_unlocked(logline, len);
    }
    else {
        append_unlocked(logline, len);
    }
}

void LogFile::flush() {
    if (mutex_) {       // 线程安全时
        MutexLockGuard lock(*mutex_);
        file_->flush();
    }
    else {
        file_->flush();
    }
}


void LogFile::append_unlocked(const char* logline, int len) {
    file_->append(logline, len);
    // 判定是否需要滚动日志
    if (file_->writtenBytes() > rollSize_) {        // 写入的字节数超过了rollSize
        rollFile();
    }
    else {
        if (count_ > kCheckTimeRoll_) {
            count_ = 0;
            time_t now = ::time(NULL);
            time_t thisPeriod_ = now / kRollPerSeconds_ * kRollPerSeconds_;
            if (thisPeriod_ != startOfPeriod_) {        // 如果!=成立，那就是第二天的零点，则日志滚动
                rollFile();
            }
            else if (now - lastFlush_ > flushInterval_) {
                lastFlush_ = now;
                file_->flush();
            }
        }
        else {
            ++count_;
        }
    }
}

LogFile::rollFile() {
    time_t now = 0;
    string filename = getLogFileName(basename_, &now);
    time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;
    // 注意，这里先除kRollPerSeconds_后乘kRollPerSeconds_的原因是表示
    // 对齐至kRollPerSeconds_整数倍，也就是时间调整到当天零点。

    if (now > lastRoll_) {// 滚动一个日志，产生一个新的日志文件
        lastRoll_ = now;
        lastFlush_ = now;
        startOfPeriod_ = start;
        file_.reset(new File(filename));

    }
}

string LogFile::getLogFileName(const string& basename, time_t* now) {
    string filename;
    filename.reserve(basename.size() + 64);// 保留（文件名称+64）个字节的空间
    filename = basename;

    char timebuf[32];
    char pidbuf[32];
    struct tm tm;
    *now time(NULL);// 距离1970年1月1日0点的秒数保存到now当中
    gmtime_r(now, &tm);// gmtime_r相对于gmtime来说是线程安全的;
    // FIXME: localtime_r?
    strftime(tiembuf, sizeof timebuf, "。%Y%m%d-%H%M%S.", &tm);
    filename += timebuf;
    filename += ProcessInfo::hostname();
    snprintf(pidbuf, sizeof pidbuf, ".%d", ProcessInfo::pid());
    filename += pidbuf;
    filename += ".log";

    return filename;
}

