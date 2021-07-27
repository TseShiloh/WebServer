#include <muduo/base/Timestamp.h>

#include <sys/time.h>
#include <stdio.h>
#define __STD_FORMAT_MACROS
#include <inttypes.h>
#undef __STDC_FORMAT_MACROS

#include <boost/static_assert.hpp>

using namespace muduo;

// assert是运行时断言，BOOST_STATIC_ASSERT是编译时断言
BOOST_STATIC_ASSERT(sizeof(Timestamp) == sizeof(int64_t));

Timestamp::Timestamp(int64_t microseconds) : microSecondsSinceEpoch_(microseconds) 
{
}

string Timestamp::toString() const {
    char buf[32] = {0};
    int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
    int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
    snprintf(buf, sizeof(buf)-1, "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
    return buf;
}
    /* 
    int64_t，在32位中是long long int，在64位中是long int，所以打印int64_t的格式化方法是：
    printf("%ld", value);    //64bit OS
    printf("%lld", value);   //32bit OS
    
    跨平台的用法:
    #define__STDC_FORMAT_MACROS
    #include<inttypes.h>
    #undef__STDC_FORMAT_MACROS
    printf("%" PRId64 "\n", value);
    */

string Timestamp::toFormattedString() const {
    char buf[32] = {0};
    time_t seconds = static_cast<time_t>(microSecondsSince_ / kMicroSecondsPerSecond);
    int microseconds = static_cast<int>(microSecondsSince_ % kMicroSecondsPerSecond);
    struct tm tm_time;
    gmtime_r(&seconds, &tm_time);

    snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d", // 格式化为字符串
            tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday, 
            tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, microseconds);
    return buf;
}

Timestamp Timestamp::now() {
    struct timeval tc;
    gettimeofday(&tv, NULL);
    int64_t seconds = tv.tv_sec;
    return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec)
}

Timestamp Timestamp::invalid() {
    return Timestamp();
}