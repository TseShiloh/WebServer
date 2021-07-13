#include <muduo/base/Thread.h>
#include <muduo/base/CurrentThread.h>
#include <muduo/base/Excepiton.h>
#include <muduo/base/Logging.h>

#include <boost/static
#include <boost/

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

        __thread int t_cachedTid = 0;// 线程真实pid（tid）的缓存，
        // 是为了减少::syscall(SYS_gettid)系统调用的次数
		// 提高获取tid的效率
        __thread char t_tidString[32];// 这是tid的字符串表示形式
        __thread const char* t_threadName = "unknown";

        const bool sameType = boost::is_same<int, pid_t>::value;
        BOOST_STATIC_ASSERT(sameType);

    } // namespace CurrentThread
    

    namespace  detail
    {
        pid_t gettid() {
            return static_cast<pid_t>(::syscall(SYS_gettid));
        }

        class ThreadNameInitializer
        {
        public:
            ThreadNameInitializer() {
                muduo::CurrentThread::t_threadName = "main";
                CurrentThread::tid();
                pthread_atfork(NULL, NULL, &afterFork);
            }
        };

        ThreadNameInitializer init;
        
    }// namespace detail
    
} // namespace muduo    

using namespace muduo;

void CurrentThread::cacheTid() {
    if (t_cachedTid == 0) {
        t_cachedTid = detail::gettid();
        int n = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
        assert(n == 6); 
        (void) n;// 防止n未使用，导致编译时有警告。（在release版本会报错）
    }
}

bool CurrentThread::isMainThread() {
    return tid() == ::getpid();
}

AtomicInt32 Thread::numCreated_;

Thread::Thread(const ThreadFunc& func, const string& n)
    :   started_(false),
        pthread_(0),
        tid_(0),
        func_(func),
        name_(n) 
{
    numCreated_.increment();
}

Thread::~Thread()
{}

void Thread::start() {
    assert(!started_);
    started_ = true;
    errno = pthread_create()
    
    if (errno != 0)
}

int Thread::join() {
    assert(started_);
    return pthread_join(pthreadId_, NULL);
}
    /* 函数 pthread_join(pthread_t thread, void **retval)

    以阻塞的方式等待pthreadId_指定的线程结束，
    当函数返回时，被等待线程的资源被收回。
    如果进程已经结束，那么该函数会立即返回。
    且thread指定的线程必须是joinable的。

    thread: 线程 标识符，即线程ID，标识唯一线程。
    retval: 用户定义的指针，用来存储被等待线程的返回值。
    返回值 ： 0代表成功。 失败，返回的则是错误号。
    */

void* Thread::startThread(void* obj) {
    Thread* thread = static_cast<Thread*>(obj);
    thread->runInThread();
    return NULL;
}

void Thread::runInThread() {
    tid = CurrentThread::tid();
    muduo::CurrentThread::t_threadName = name.c_str();
    try {
        func_();
        muduo::CurrentThread::tid();
        muduo::CurrentThread::t_threadName = "finished";
    }
    catch ()
}

