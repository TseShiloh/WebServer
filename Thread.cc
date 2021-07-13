#include <muduo/base/Thread.h>
#include <muduo/base/CurrentThread.h>
#include <muduo/base/Excepiton.h>
#include <muduo/base/Logging.h>

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>

namespace muduo
{

    namespace CurrentThread
    {
        //thread修饰的变量是全局的，所有线程共享。
        // __thread修饰的变量是线程局部存储的，每个线程都有一份。

        __thread int t_cachedTid = 0; // 线程真实pid（tid）的缓存，
        // 是为了减少::syscall(SYS_gettid)系统调用的次数
        // 提高获取tid的效率
        __thread char t_tidString[32]; // 这是tid的字符串表示形式
        __thread const char *t_threadName = "unknown";

        const bool sameType = boost::is_same<int, pid_t>::value;
        BOOST_STATIC_ASSERT(sameType);

    } // namespace CurrentThread

    namespace detail
    {
        pid_t gettid()
        {
            return static_cast<pid_t>(::syscall(SYS_gettid));
        }

        void afterFork()
        {
            muduo::CurrentThread::t_cacheTid = 0;
            muduo::CurrentThread::t_threadName = "main";
            CurrentThread::tid();
        }
        /*
            在fork之前可能有多个线程，
            fork可能是主线程调用，也可能是子线程中调用
            如果是后者，fork得到一份新进程（该进程只有一个子线程：调用fork的线程将会被继承下来）
            这个线程是新进程的主线程，改名、改tid、缓存tid

        */

        class ThreadNameInitializer
        {
        public:
            ThreadNameInitializer()
            {
                muduo::CurrentThread::t_threadName = "main";
                CurrentThread::tid();
                pthread_atfork(NULL, NULL, &afterFork); // 如果调用fork函数成功后，子进程会调用afterfork
            }
        };

        ThreadNameInitializer init; // 在detail的空间里是全局变量

    } // namespace detail

} // namespace muduo

using namespace muduo;

void CurrentThread::cacheTid()
{
    if (t_cachedTid == 0)
    {
        t_cachedTid = detail::gettid();
        int n = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
        assert(n == 6);
        (void)n; // 防止n未使用，导致编译时有警告。（在release版本会报错）
    }
}

bool CurrentThread::isMainThread()
{
    return tid() == ::getpid();
}

AtomicInt32 Thread::numCreated_;

Thread::Thread(const ThreadFunc &func, const string &n)
    : started_(false),
      pthread_(0),
      tid_(0),
      func_(func),
      name_(n)
{
    numCreated_.increment();
}

Thread::~Thread()
{
}

void Thread::start()
{
    assert(!started_);
    started_ = true;
    errno = pthread_create(&pthreadId_, NULL, &startThread, this);
    if (errno != 0)
    {
        LOG_SYSFATAL << "Failed in pthread_create";
    }
}

int Thread::join()
{
    assert(started_);
    return pthread_join(pthreadId_, NULL);
}
/* 函数 pthread_join(pthread_t thread, void **retval)

    以阻塞的方式等待thread指定的线程结束，
    当函数返回时，被等待线程的资源被收回。
    如果进程已经结束，那么该函数会立即返回。
    且thread指定的线程必须是joinable的。

    thread: 线程 标识符，即线程ID，标识唯一线程。
    retval: 用户定义的指针，用来存储被等待线程的返回值。
    返回值 ： 0代表成功。 失败，返回的则是错误号。
*/

void *Thread::startThread(void* obj)
{
    Thread* thread = static_cast<Thread*>(obj);
    thread->runInThread();
    return NULL;
}

void Thread::runInThread() {
    tid = CurrentThread::tid();
    muduo::CurrentThread::t_threadName = name.c_str();
    try {
        func_();
        muduo::CurrentThread::t_threadName = "finished";
    }
    catch (const Exception& ex) {
        muduo::CurrentThread::t_threadName = "crashed";
        fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
        abort();
    }
    catch (const std::exception& ex) {
        muduo::CurrentThread::t_threadName = "crashed";
        fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        abort();
    }
    catch (...) {
        muduo::CurrentThread::t_threadName = "crashed";
        fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
        throw;// rethrow
    }
}

/*  abort()函数
    其典型实现是向标准错误流发送 abnormal program termination(程序异常终止)，
    然后终止程序。
    调用Abort()时，不进行任何清理工作，直接终止程序
*/