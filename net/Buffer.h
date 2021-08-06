#ifndef MUDUO_NET_BUFFER_H
#define MUDUO_NET_BUFFER_H

#include <WebServer/base/copyable.h>
#include <WebServer/base/StringPiece.h>
#include <WebServer/base/Types.h>

#include <WebServer/net/Endian.h>

#include <algorithm>
#include <vector>

#include <assert.h>
#include <string.h>
//#include <unistd.h>  // ssize_t
/// 11111111
namespace muduo
{
    namespace net
    {

        /// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
        ///
        /// @code
        /// +-------------------+------------------+------------------+
        /// | prependable bytes |  readable bytes  |  writable bytes  |
        /// |                   |     (CONTENT)    |                  |
        /// +-------------------+------------------+------------------+
        /// |                   |                  |                  |
        /// 0      <=      readerIndex   <=   writerIndex    <=     size
        /// @endcode
        class Buffer : public muduo::copyable
        {
        private:
            std::vector<char> buffer_;	// vector用于替代固定大小数组
            size_t readerIndex_;			// 读位置
            size_t writerIndex_;			// 写位置

            static const char kCRLF[];	// "\r\n"

        public:
            static const size_t kCheapPrepend = 8;
            static const size_t kInitialSize = 1024;

            Buffer()
                : buffer_(kCheapPrepend + kInitialSize),
                  readerIndex_(kCheapPrepend),
                  writerIndex_(kCheapPrepend)
            {
                assert(readableBytes() == 0);
                assert(writableBytes() == kInitialSize);
                assert(prependableBytes() == kCheapPrepend);
            }

            // default copy-ctor, dtor and assignment are fine

            void swap(Buffer& rhs) {
                buffer_.swap(rhs.buffer_);
                std::swap(readerIndex_, rhs.readerIndex_);
                std::swap(writerIndex_, rhs.writerIndex_);
            }

            size_t readableBytes() const
            { return writerIndex_ - readerIndex_; }

            size_t writableBytes() const
            { return buffer_.size() - writerIndex_; }

            size_t prependableBytes() const
            { return readerIndex_; }

            const char* peek() const// 返回读的指针
            { return begin() + readerIndex_; }

            // 查找"\r\n"
            const char* findCRLF() const {
                const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF + 2)；
                return crlf = beginWrite() ? NULL : crlf;
            }

            const char* findCRLF(const char* start) const
            {
                assert(start >= peek());
                assert(start <= beginWrite());
                const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF + 2);
                return crlf = beginWrite() ? NULL : crlf;
            }

            // retrieve returns void, to prevent
            // string str(retrieve(readableBytes()), readableBytes());
            // the evaluation of two functions are unspecified
            void retrieve(size_t len)
            {
                assert(len <= readableBytes());
                if (len < readableBytes()) {    // 只取回部分数据
                    readerIndex_ += len;        // 则只需偏移len
                }
                else {                          // 取回所有数据
                    retrieveAll();              // 则readerIndex_和writerIndex_都重置
                }
            }

            void retrieveUntil(const char* end)// 取回数据直到某一个位置
            {
                assert(end >= peek());
                assert(end <= beginWrite());
                retrieve(end - peek());
            }

            void retrieveInt32()    // 取回4个字节
            {
                retrieve(sizeof(int32_t));
            }

            void retrieveInt16()    // 取回2个字节
            {
                retrieve(sizeof(int16_t));
            }

            void retrieveInt8()     // 取回1个字节
            {
                retrieve(sizeof(int8_t));
            }

            void retrieveAll()// 重置readerIndex_和writerIndex_
            {
                readerIndex_ = kCheapPrepend;
                writerIndex_ = kCheapPrepend;
            }

            string retrieveAllAsString()
            {
                return retrieveAsString(readableBytes());
            }

            string retrieveAsString(size_t len)
            {
                assert(len <= readableBytes());
                string result(peek(), len);// 取出数据
                retrieve(len);// 偏移readerIndex_
                return result;
            }

            StringPiece toStringPiece() const
            {
                return StringPiece(peek(), static_cast<int>(readableBytes()));
            }

            void append(const StringPiece& str)
            {
                append(str.data(), str.size());
            }

            void append(const char* /*restrict*/ data, size_t len)
            {
                ensureWritableBytes(len);// 确保缓冲区可写空间>=len
                std::copy(data, data+len, beginWrite());
                hasWritten(len);
            }

            void append(const void* /*restrict*/ data, size_t len)
            {
                append(static_cast<const char*>(data), len);
            }

            // 确保缓冲区可写空间>=len，如果不足则扩充
            void ensureWritableBytes(size_t len)
            {
                if (writableBytes() < len) {
                    makeSpace(len);
                }
                assert(writableBytes() >= len);
            }

            char* beginWrite()
            { return begin() + writerIndex_; }

            const char* beginWrite() const
            { return begin() + writerIndex_; }

            void hasWritten(size_t len)
            { writerIndex_ += len; }

            ///
            /// Append int32_t using network endian
            ///
            void appendInt32(int32_t x)
            {
                int32_t be32 = sockets::hostToNetwork32(x);
                append(&be32, sizeof be32);
            }

            void appendInt16(int16_t x)
            {
                int16_t be16 = sockets::hostToNetwork16(x);
                append(&be16, sizeof be16);
            }

            void appendInt8(int8_t x)
            {
                append(&x, sizeof x);
            }

            ///
            /// Read int32_t from network endian
            ///
            /// Require: buf->readableBytes() >= sizeof(int32_t)
            int32_t readInt32()
            {
                int32_t result = peekInt32();
                retrieveInt32();
                return result;
            }

            int16_t readInt16()
            {
                int16_t result = peekInt16();
                retrieveInt16();
                return result;
            }

            int8_t readInt8()
            {
                int8_t result = peekInt8();
                retrieveInt8();
                return result;
            }

            ///
            /// Peek int32_t from network endian
            ///
            /// Require: buf->readableBytes() >= sizeof(int32_t)
            int32_t peekInt32() const
            {
                assert(readableBytes() >= sizeof(int32_t));
                int32_t be32 = 0;
                ::memcpy(&be32, peek(), sizeof be32);
                return sockets::networkToHost32(be32);
            }

            int16_t peekInt16() const
            {
                assert(readableBytes() >= sizeof(int16_t));
                int16_t be16 = 0;
                ::memcpy(&be16, peek(), sizeof be16);
                return sockets::networkToHost16(be16);
            }

            int8_t peekInt8() const
            {
                assert(readableBytes() >= sizeof(int8_t));
                int8_t x = *peek();
                return x;
            }

            ///
            /// Prepend int32_t using network endian
            ///
            void prependInt32(int32_t x)
            {
                int32_t be32 = sockets::hostToNetwork32(x);
                prepend(&be32, sizeof be32);
            }

            void prependInt16(int16_t x)
            {
                int16_t be16 = sockets::hostToNetwork16(x);
                prepend(&be16, sizeof be16);
            }

            void prependInt8(int8_t x)
            {
                prepend(&x, sizeof x);
            }

            void prepend(const void* /*restrict*/ data, size_t len)
            {
                assert(len <= prependableBytes());
                readerIndex_ -= len;
                const char* d = static_cast<const char*>(data);
                std::copy(d, d+len, begin()+readerIndex_);
            }

            void shrink(size_t reserve)// 收缩，保留reserve个字节
            {
                // FIXME: use vector::shrink_to_fit() in C++ 11 if possible.
                Buffer other;
                other.ensureWritableBytes(readableBytes() + reserve);
                other.append(toStringPiece());
                swap(other);
            }

            /// Read data directly into buffer.
            ///
            /// It may implement with readv(2)
            /// @return result of read(2), @c errno is saved
            ssize_t readFd(int fd, int* savedErrno);


        private:
            char* beginWrite()
            { return begin() + writerIndex_; }

            const char* beginWrite() const
            { return begin() + writerIndex_; }

            void makeSpace(size_t len)
            {
                if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
                    buffer_.resize(writerIndex_ + len);
                }
                else {
                    assert(kCheapPrepend < readerIndex_);
                    size_t readable = readableBytes();
                    //将原来可读部分[begin()+readerIndex_, begin()+writerIndex_)向前拷贝到[begin(), begin()+kCheapPrepend)
                    std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + kCheapPrepend);
                    // 重置readerIndex_和writerIndex_
                    readerIndex_ = kCheapPrepend;
                    writerIndex_ = readerIndex_ + readable;
                    assert(readable == readableBytes());
                }
            }
            
        }; // class Buffer
    } // namespace net
} // namespace muduo

#endif  // MUDUO_NET_BUFFER_H
