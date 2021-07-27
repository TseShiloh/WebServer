// 线程本地存储类


/* 线程特定数据TSD
有时候需要提供线程私有的全局变量，仅在某个线程中有效，但是可以跨多个函数访问。
POSIX线程库通过维护一定的数据结构来解决这个问题，这个数据为Thread-special Data(TSD)
线程特定数据也称为线程本地存储TLS(Thread-local storage)
对于POD类型的线程本地存储，可以用__thread关键字
*/

/* POSIX线程库通过以下函数操作线程特定数据
pthread_key_create
pthread_key_delete
pthread_getspecific
pthread_setspecific
*/

#ifndef MUDUO_BASE_THREADLOCAL_H
#define MUDUO_BASE_THREADLOCAL_H

#include <boost/noncopyable.hpp>
#include <pthread.h>

namespace muduo
{
    template<typename T>
    class ThreadLocal : boost::noncopyable
    {
    private:
        pthread_key_t pkey_;
        static void destructor(void* x);// 用来销毁实际数据的回调函数
    
    public:
        ThreadLocal();
        ~ThreadLocal();

        T& value();     // 实际的特定数据
    };

    ThreadLocal::ThreadLocal() {
        // 删除实际数据需要在内指定一个回调函数destructor
        pthread_key_creat(&pkey_, &ThreadLocal::destructor);
    }

    ThreadLocal::~ThreadLocal() {
        pthread_key_delete(pkey_);// 删除的是指针,而不是实际数据。实际数据在堆上
    }

    static void destructor(void* x) {
        T* obj = static_cast<T*>(x);
        typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
        delete obj;
    }
    
    T& value() {
        T* perThreadValue = static_cast<T*>(pthread_getspecific(pkey_));
        // 获取线程特定数据perThreadValue
        if (!perThreadValue) {      // 如果指针为空，即特定数据还没被创建
            T* newObj = new T();    // 那么就创建这个数据
            pthread_setspecific(pkey_, newObj);
            perThreadValue = newObj;
        }
        return *perThreadValue;
    }


} // namespace muduo

#endif // MUDUO_BASE_THREADLOCAL_H


