// 缓冲区类FixedBuffer

#ifndef MUDUO_BASE_LOGSTREAM_H
#define MUDUO_BASE_LOGSTREAM_H

#include <muduo/base/StringPiece.h>
#include <muduo/base/Types.h>
#include <assert.h>
#include <string.h> // memcpy
#ifndef MUDUO_STD_STRING
#include <string>
#endif

#include <boost/noncopyable.hpp>

namespace muduo
{
    namespace detail
    {
        const int kSmallBuffer = 4000;
        const int kLargeBuffer = 4000*1000;

        template<int SIZE>// SIZE为非类型参数，传递的是一个值
        class FixedBuffer : boost::noncopyable
        {
        private:
            const char* end() const { return data_ + sizeof data_; }
            // Must be outline function for cookies.
            static void cookieStart();
            static void cookieEnd();

            void (*cookie_)();
            char data_[SIZE];// 缓冲区
            char* cur_;// 当前指针

        public:
            FixedBuffer()
                : cur_(data_)
            {
                setCookie(cookieStart);
            }

            ~FixedBuffer()
            {
                setCookie(cookieEnd);
            }

            void append(const char* buf, size_t len) {
                // FIxME: append partially当前空间不够，自行实现部分添加功能。
                if (implicit_cast<size_t>(avail()) > len) {// 当前可用空间大于len
                    memcpy(cur_, buf, len);// 将buf添加进去
                    cur_ += len;
                }
            }

            const char* data() const { return data_; }
            int length() const { return static_cast<int>(cur_ - data_); }

            // write to data_ directly
            char* current() { return cur_; }
            int avail()  const { return static_cast<int>(end() - cur_); }
            void add(size_t len) { cur_ += len; }

            void reset() { cur_ = data; }// 重置，将cur_指向回首地址
            void bzero() { ::bzero(data_, sizeof data_); }// 将数据清为0

            // for used by GDB
            const char* debugString();// 在buffer后面加个"\0"，将其当成字符串
            void setCookie(void (*cookie)()) { cookie_ = cookie; }
            // for used by unit test
            string asString() const { return string(data_, length()); }

        }; // class FixedBuffer

    } // namespace detail

    class LogStream : boost noncopyable
    {
        typedef LogStream self;// 定义自身
        
    private:
        void staticCheck();
        template<typename T>
        void formatInteger(T);

        Buffer buffer_;

        static const int kMaxNumericSize = 32;

    public:
        typedef detail::FixedBuffer<detail::kSmallBuffer> Buffer;// 输出时先输出到Buffer这个缓冲区

        self& operator<<(bool v) {
            buffer_.append(v ? "1" : "0", 1);
            return *this;
        }

        self& operator<<(short);
        self& operator<<(unsigned short);
        self& operator<<(int);
        self& operator<<(unsigned int);
        self& operator<<(long);
        self& operator<<(unsigned long);
        self& operator<<(long long);
        self& operator<<(unsigned long long);

        self& operator<<(const void*);

        self& operator<<(float v) {
            *this << static_cast<double>(v);
            return *this;
        }

        self& operator<<(double);
        // self& operator<<(long double);

        self& operator<<(char v) {
            buffer_.append(&v, 1);
            return *this;
        }

        // self& operator<<(signed char);
        // self& operator<<(unsigned char);

        self& operator<<(const char* v) {
            buffer_.append(v, strlen(v));
            return *this;
        }

        self& operator<<(const string& v) {// 短字符优化的字符串
            buffer_.append(v.c_str(), v.size());
            return *this;
        }

        #ifndef MUDUO_STD_STRING
        self& operator<<(const std::string& v) {// STL中的字符串
            buffer_.append(v.c_str(), v.size());
            return *this;
        }
        #endif

        self& operator<<(const StringPiece& v) {// 也可以吧StringPiece看成字符串
            buffer_.append(v.data(), v.size());
            return *this;
        }

        void append(const char* data, int len) { buffer_.append(data, len); }
        const Buffer& buffer() const { return buffer_; }
        void resetBuffer() { buffer_.reset(); }

    };// class LogStream
    
    class Fmt // : boost::noncopyable
    {
    private:
        char buf_[32];
        int length_;

    public:
        template<typename T>
        Fmt(const char* fmt, T val);// 把整数val按照fmt的格式进行格式化到buf_中

        const char* data() const { return buf_; }
        int length() const { return length_; }
    };

    inline LogStream& operaotr<<(LogStream& s, const Fmt& fmt) {
        s.append(fmt.data(), fmt.length());
        return s;
    }

} // namespace muduo

#endif // MUDUO_BASE_LOGSTREAM_H