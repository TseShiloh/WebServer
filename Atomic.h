#ifndef MUDUO_BASE_ATOMIC_H
#define MUDUO_BASE_ATOMIC_H

#include <boost/noncopyable.hpp>
#include <stdint.h>

namespace muduo 
{
    namespace detail
    {
        template<typename T>
        class AtomicIntergeT : boost::noncopyable 
        {
            public:
            AtomicIntergeT() : value_(0) 
            {}

            /// 拷贝构造
            /// @param that 被拷贝的对象 
            AtomicIntergeT(const AtomicIntergeT& that) : value_(that.get())
            {}

            AtomicIntergeT& operator=(const AtomicIntergeT& that) {
                getAndSet(that.get());
                return *this;
            }
            ///
            
            T get() {
                return _sync_val_compare_and_swap(&value_, 0, 0);// CAS操作：原子比较与设置
            }

            T getAndAdd(T x) {
                return _syn_fetch_and_add(&value_, x);
            }

            T addAndGet(T x) {
                return getAndAdd(x) + x;
            }

            T incrementAndGet() {//自增函数
                return addAndGet(1);
            }

            T decrementAndGet() {//自减函数
                return addAndGet(-1);
            }

            void add(T x) {
                getAndAdd(x);
            }

            void increment() {
                incrementAndGet();
            }

            void decrement() {
                decrementAndGet();
            }

            T getAndSet(T newValue) {
                return _sync_lock_test_and_set(&value_, newValue);
            }

            private:
            volatile T value_;
            /*
            volatile可以确保本条指令不会因编译器的优化而省略
            当要求使用volatile声明的变量值时，
            系统总是直接从它所在的内存读取数据，
            而不是使用保存在寄存器中的备份。
            /*
        };// end AtomicIntergeT?
    }// end detail?

    typedef detail::AtomicIntergeT<int32_t> AtomicInt32;
    typedef detail::AtomicIntergeT<int64_t> AtomicInt64;
}// end muduo?

#endif // MUDUO_BASE_ATOMIC_H