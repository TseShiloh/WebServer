#ifndef MUDUO_BASE_LOGFILE_H
#define MUDUO_BASE_LOGFILE_H

#include <muduo/base/Mutex.h>
#include <muduo/base/Types.h>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

namespace muduo
{
	class LogFile : boost::noncopyable
	{
	public:
		LogFile(const string& basename,
				size_t rollSize,
				bool threadSafe = true,		// 线程安全默认为true
				int flushInterval = 3);		// flush的时间间隔为3秒钟
		
        ~LogFile();

		void append(const char* logline, int len);		// 将logline这一行长度len添加到日志文件中
		void flush();		// 清空缓冲区

	private:
		void append_unlocked(const char* logline, int len);		// 非线程安全的添加日志

		static string getLogFileName(const string& basename, time_t* now);	// 获取日志文件名
		void rollFile();		// 滚动日志
		
		const string basename_;			// 日志文件basename_
		const size_t rollSize_;			// 日志文件大小达到rollSize_换一个新文件
		const int flushInterval_;		// 日志写入时间间隔flushInterval_
	
		int count_;			// 计数器，达到kCheckTimeRoll_时检测一下是否需要换一个新的日志文件；或者要将日志写进实际的文件当中

		boost::scoped_ptr<MutexLock> mutex_;	// 互斥量
		time_t startOfPeriod_;		// 开始记录日志的时间（调整至零点的时间）
		time_t lastRoll_;			// 上一次滚动日志文件时间
		time_t lastFlush_;			// 上一次日志写入文件时间
		class File;					// File是嵌套类，进行一个前向声明
		boost::scoped_ptr<File> file_;

		const static int kCheckTimeRoll_ = 1024;			// 检查“是否日志滚动”的计时
		const static int kRollPerSeconds_ = 60*60*24;		// 一天的计时
	}; // class LogFile
} // namespace muduo

#endif // MUDUO_BASE_LOGFILE_H

