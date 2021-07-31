/*
1. Endian.h
- 封装了字节序转换函数（全局函数，位于muduo::net::sockets名称空间中）。
*/
#ifndef MUDUO_NET_ENDIAN_H
#define MUDUO_NET_ENDIAN_H

#include <stdint.h>
#include <endian.h>

namespace muduo
{
    namespace net
    {
        namespace sockets
        {
            #if __GNUC_MINOR__ >= 6;
            #pragma GCC diagnostic push
            #endif
            #pragma GCC diagnostic ignored "-Wconversion"
            #pragma GCC diagnostic ignored "-Wold-style-cast"
            
            // be是big endian的意思
            // htonl（32位），htons（16位）是proxy标准中的函数。（可移植）
            // 但htobe64等是不可移植的
            inline uint64_t hostToNetwork64(uint64_t host64) {
                return htobe64(host64);
            }
            inline uint32_t hostToNetwork32(uint32_t host32) {
                return htobe32(host32);
            }
            inline uint16_t hostToNetwork16(uint16_t host16) {
                return htobe16(host16);
            }

            inline uint64_t networkToHost64(uint64_t net64) {
                return be64toh(net64);
            }
            inline uint32_t networkToHost32(uint32_t net32) {
                return be32toh(net32);
            }
            inline uint16_t networkToHost16(uint16_t net16) {
                return be16toh(net16);
            }
            
        } // namespace sockets
    } // namespace net
} // namespace muduo

#endif // MUDUO_NET_ENDIAN_H