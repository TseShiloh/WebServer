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

#include <muduo/base/LogStream.h>
#include <muduo/base/TimeStamp.h>

namespace muduo
{
    class Logger
    {
    public:
        enum LogLevel {
            TRACE,// 0
            DEBUG,// 1
            INFO,// 2
            WARN,// 3
            ERROR,// 4
            FATAL,// 5
            NUM_LOG_LEVELS,// 级别的个数，这里刚好等于6
        };
        
        class SourceFile
        {
        public:
            const char* data_;
            int size_;

            template<int N>
            inline SourceFile(const char (&arr)[N])
                : data_(arr)
                  size_(N-1)
            {
                const
            }

            SourceFile(/* args */);
            ~SourceFile();
        };

        Logger(SourceFile file, int line);
        Logger(SourceFile file, int line, LogLevel level);
        Logger(SourceFile file, int line, LogLevel level, const char* func);
        Logger(SourceFile file ,int line, bool toAbort);
        ~Logger();

        LogStream& stream() {return impl_.stream_; }// stream实际是放在Impl类里
        
        class Impl
        {
        public:
            typedef Logger::LogLevel LogLevel;
            Impl(LogLevel level, int old_error, const SourceFile& file, int line);
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


} // namespace muduo
extern Logger::LogLevel g_loglevel;

inline Logger::LogLevel Logger::logLevel() {
    return g_logLevel;
}
    
#define LOG_TRACE if (muduo::Logger::logLevel() <= muduo::Logger::TRACE) \
    muduo::Logger(__FILE__, __LINE__, muduo::Logger::TRACE, __func__).stream()
#define LOG_DEBUG if (muduo::Logger::logLevel() <= muduo::Logger::DEBUG) \
    muduo::Logger(_ _FILE__, __LINE__, muduo::Logger::DEBUG, __func__).stream()
#define LOG_INFO  if (muduo::Logger::logLevel() <= muduo::Logger::INFO) \
    muduo::Logger::(__FILE__, __LINE__).stream()// 构造匿名的Logger对象调用stream()
#define LOG_WARN muduo::Logger(__FILE__, __LINE__, muduo::Logger::WARN).stream()
#define LOG_ERROR muduo::Logger(__FILE__, __LINE__, muduo::Logger::ERROR).stream()
#define LOG_FATAL muduo::Logger(__FILE__, __LINE__, muduo::Logger::FATAL).stream()
#define LOG_SYSERR muduo::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL muduo::Logger(__FILE__, __LINE__, true).stream()

const char* strerror_tl(int savedErrno);

#define CHECK_NOTNULL(val) \ ::muduo::CheckNotNull(__FIEL__, __LINE__, )


#endif // MUDUO_BASE_LOGGING_H

/*
Logger -> Impl -> LogStream -> operator<<FixedBuffer -> g_output -> g_flush

Logger类内部包装了Impl类，Logger类负责了日志的级别，是最外层的日志类
Impl类是内部实际的实现，比如如何格式化日志
借助LogStream来输出日志，输出到缓冲区FixedBuffer
g_output决定输出到设备还是文件
g_flush，输出（清空标准输出的缓冲区）

*/


