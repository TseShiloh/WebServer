#include <muduo/base/Logging.h>
#include <muduo/base/LogFile.h>
#include <muduo/base/ThreadPool.h>

#include <stdio.h>

int g_total;
FILE* g_file;// 初始值g_file为空
boost::scoped_ptr <muduo::LogFile> g_logFile;// 智能指针g_logFile，初始值为空

void dummyOutput(const char* msg, int len) {
    g_total += len;
    if (g_file) {
        fwrite(msg, 1, len, g_file);
    }
    else if (g_logFile) {
        g_logFile->append(msg, len);
    }
}

void bench(const char*  type) {
    muduo::Logger::setOutput(dummyOutput);
    muduo::Timestamp start(muduo::Timestamp::now());
    g_total = 0;

    int n = 1000*1000;
    const bool kLongLog = false;
    muduo::string empty =  " ";
    muduo::string longStr(3000, 'X');
    longStr += " ";
    for (int i = 0; i < n; ++i) {
        LOG_INFO << "Hello 0123456789 " 
                 << "abcdefghijklmnopqrstuvwxyz"
                 << (kLonglog ? longStr : empty)
                 << i;
    }
    muduo::Timestamp end(muduo::Timestamp::now());
    double seconds = timeDifference(end, start);
    printf("%12s: %f seconds, %d bytes, %10.2f msg/s, %.2f MiB/s\n",
          type, seconds, g_total, n / seconds, g_total / seconds / (1024*1024));
}

void logInThread() {
    LOG_INFO << "logInThread";// 线程池的线程在运行
    usleep(1000);
}

int main() {
    getppid();

    muduo::ThreadPool pool("pool");
    pool.start(5);
    // 运行了5个任务，这5个任务是线程池中的线程运行的，不是主线程运行的。
    pool.run(logInThread);
    pool.run(logInThread);
    pool.run(logInThread);
    pool.run(logInThread);
    pool.run(logInThread);

    // 主线程的输出
    LOG_TRACE << "Trace";
    LOG_DEBUG << "Debug";
    LOG_INFO  << "Info";
    LOG_WARN  << "Warn";
    LOG_ERROR << "Error";
    LOG_INFO  << sizeof(muduo::Logger);
    LOG_INFO  << sizeof(muduo::LogStream);
    LOG_INFO  << sizeof(muduo::Fmt);
    LOG_INFO  << sizeof(muduo::LogStream::Buffer);

    sleep(1);
    bench("nop");// 性能测试程序

    char buffer[64*1024];

    g_file = fopen("/dev/null", "w");
    setbuffer(g_file, buffer, sizeof buffer);
    bench("/dev/null");
    fclose(g_file);

    g_file = fopen("/tmp/log", "w");
    setbuffer(g_file, buffer, sizeof buffer);
    bench("/tmp/log");
    fclose(g_file);

    g_file = NULL;
    // 下面用（单线程test_log_st，，非线程安全）的方式写入
    g_logFile.reset(new muduo::LogFile("test_log_st", 500*1000*1000, false));
    bench("test_log_st");

    g_logFile.reset(new muduo::LogFile("test_log_mt", 500*1000*1000, true));
    // 下面用（多线程test_log_mt，，线程安全）的方式写入
    bench("test_log_mt");
    g_logFile.reset();
}