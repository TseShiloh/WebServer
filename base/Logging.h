/* 日志级别
1.TRACE 指出比DEBUG粒度更细的一些信息事件
2.DEBUG 指出细粒度信息事件对调试应用程序时非常有帮助的
3.INFO  表明消息在粗粒度级别上，突出强调应用程序的运行过程
4.WARN  系统能正常运行，但可能会出现潜在错误的情形
5.ERROR 指出虽然发生错误事件，但仍然不影响系统的继续运行
6.FATAL 指出每个严重的错误事件将会导致应用程序的退出

*/

#ifndef MUDUO_BASE_LOGGING_H
#define MUDUO_BASE_LOGGING_H

#include <WebServer/base/LogStream.h>
#include <WebServer/base/TimeStamp.h>

namespace muduo
{
    class Logger
    {
    public:
        enum LogLevel
        {
            TRACE,          // 0
            DEBUG,          // 1
            INFO,           // 2
            WARN,           // 3
            ERROR,          // 4
            FATAL,          // 5
            NUM_LOG_LEVELS, // 级别的个数，这里刚好等于6
        };                  // enum LogLevel

        class SourceFile
        {
        public:
            const char *data_;
            int size_;

            template <int N>
            inline SourceFile(const char (&arr)[N])
                : data_(arr),
                  size_(N - 1)
            {
                const char *slash = strrchr(data_, '/'); // builtin function
                if (slash)
                {
                    data_ = slash + 1;
                    size_ -= static_cast<int>(data_ - arr);
                }
            }

            explicit SourceFile(const char *filename)
                : data_(filename)
            {
                const char *slash = strrchr(filename, '/');
                if (slash)
                {
                    data_ = slash + 1;
                }
                size_ = static_cast<int>(strlen(data_));
            }

        }; // class SourceFile

        Logger(SourceFile file, int line);
        Logger(SourceFile file, int line, LogLevel level);
        Logger(SourceFile file, int line, LogLevel level, const char *func);
        Logger(SourceFile file, int line, bool toAbort);
        ~Logger();

        LogStream &stream() { return impl_.stream_; } // stream实际是放在Impl类里

        static LogLevel LogLevel();
        static void setLogLevel(LogLevel level);

        typedef void (*OutputFunc)(const cahr *msg, int len);
        typedef void (*FlushFunc)();
        static void setOutput(OutputFunc);
        static void setFlush(FlushFunc);

    private:
        class Impl
        {
        public:
            typedef Logger::LogLevel LogLevel;
            Impl(LogLevel level, int old_error, const SourceFile &file, int line);
            void formatTime();
            void finish();

            Timestamp time_;
            LogStream stream_;
            LogLevel level_;
            int line_;
            SourceFile basename_;
        }; // class Impl

        Impl impl_;

    }; // class Logger

    extern Logger::LogLevel g_loglevel;

    inline Logger::LogLevel Logger::logLevel()
    {
        return g_logLevel;
    }

    #define LOG_TRACE                                          \
        if (muduo::Logger::logLevel() <= muduo::Logger::TRACE) \
        muduo::Logger(__FILE__, __LINE__, muduo::Logger::TRACE, __func__).stream()
    #define LOG_DEBUG                                          \
        if (muduo::Logger::logLevel() <= muduo::Logger::DEBUG) \
        muduo::Logger(_ _FILE__, __LINE__, muduo::Logger::DEBUG, __func__).stream()
    #define LOG_INFO                                          \
        if (muduo::Logger::logLevel() <= muduo::Logger::INFO) \
        muduo::Logger::(__FILE__, __LINE__).stream() // 构造匿名的Logger对象调用stream()
    #define LOG_WARN muduo::Logger(__FILE__, __LINE__, muduo::Logger::WARN).stream()
    #define LOG_ERROR muduo::Logger(__FILE__, __LINE__, muduo::Logger::ERROR).stream()
    #define LOG_FATAL muduo::Logger(__FILE__, __LINE__, muduo::Logger::FATAL).stream()
    #define LOG_SYSERR muduo::Logger(__FILE__, __LINE__, false).stream()
    #define LOG_SYSFATAL muduo::Logger(__FILE__, __LINE__, true).stream()

    const char *strerror_tl(int savedErrno);

    // Taken from glog/logging.h
    //
    // Check that the input is non NULL.  This very useful in constructor initializer lists.
    #define CHECK_NOTNULL(val) \ 
        ::muduo::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))
    // (文件名称，所在行号，val变成字符串，val)

    // A small helper for CHECK_NOTNULL().
    template <typename T>
    T* CHECKNOTNULL(Logger::SourceFile file, int line, const char* names, T* ptr) {
        if (ptr == NULL) {// 如果指针为空，则登记一个FATAL日志并终止程序
            Logger(file, line, Logger::FATAL).steam() << names;
        }
        return ptr;
    }

} // namespace muduo

#endif // MUDUO_BASE_LOGGING_H

/*
Logger -> Impl -> LogStream -> operator<<FixedBuffer -> g_output -> g_flush

Logger类内部包装了Impl类，Logger类负责了日志的级别，是最外层的日志类
Impl类是内部实际的实现，比如如何格式化日志
借助LogStream来输出日志，输出到缓冲区FixedBuffer
g_output决定输出到设备还是文件
g_flush，输出（清空标准输出的缓冲区）

*/
