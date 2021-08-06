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
            std::vector<char> buffer_;  // buffer_是动态数组
            size_t readerIndex_;        // 读位置
            size_t writerIndex_;        // 写位置
        public:
            static const size_t kCheapPrepend = 8;
            static const size_t kInitialSize = 1024;,

            Buffer()
                : buffer_(kCheapPrepend + kInitialSize),
                  readerIndex_(kCheapPrepend),
                  writeindex_(kCheapPrepend)
            {
                assert(readableBytes() == 0);
                assert(writableBytes() == kInitialSize);
                assert(prependableBytes() == kCheapPrepend);
            }

            // default copy-ctor, dtor and assignment are fine

            void swap(Buffer& rhs)
            {
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


        };

    }
}


#endif MUDUO_NET_BUFFER_H