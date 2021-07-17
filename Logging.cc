#include <muduo/base/Logging.h>

#include <muduo/base/CurrentTHread.h>
#include <muduo/base/StringPiece.h>
#include <muduo/base/TimeStamp.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sstream>

namespace muduo
{

    Logger::Logger(SourceFile file, int line, LogLevel level)
        : impl_(level, 0, file, line)
    {
    }

    Logger::LogLevel initLogLevel() {
        if (::getnv("MUDUO_LOG_TRACE"))
            return Logger::TRACE;
        else if (::getnv("MUDUO_LOG_TRACE"))
            return Logger::DEBUG;
        else
            return Logger::INFO;
    }
}
using namespace muduo;

Logger::Logger(SourceFile file, int line)
    : impl_(INFO, 0, file, line)
{}

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)

{
    impl_.stream_ << func << ' ';
}
