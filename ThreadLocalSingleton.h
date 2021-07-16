// 线程本地单例类封装
// ThreadLocalSingleton<T>每一个线程都有一个T类型的单例对象

#ifndef MUDUO_BASE_THREADLOCALSINGLETON_H
#define MUDUO_BASE_THREADLOCALSINGLETON_H

#include <boost/noncopyable.hpp>
#include <assert.h>
#include <pthread.h>

namespace muduo
{
    template<typename T>
    class ThreadLocalSingleton :boost::noncopyable
    {
    private:
        static __thread T* t_value_;// __thread关键字表示每个线程都有一份t_value_
        static Deleter deleter_;// 销毁对象的函数

        static void destructor(void* obj) {
            assert(obj == t_value_);
            typedef char T_must_be_complete_type[size(T) == 0 ? -1 : 1];
            delete t_value_;
            t_value_ = 0;
        }

        class Deleter
        {
        public:
            pthread_key_t pkey_;

            Deleter() {
                pthread_key_create(&pkey_, &ThreadLocalSingleton::destructor);// 第二个参数是指定的回调函数
            }

            ~Deleter() {
                pthread_key_delete(pkey_);
            }

            void set(T* newObj) {
                assert(pthread_getspecific(pkey_) == NULL);
                pthread_getspecific(pkey_, newObj);
            }
        };
    
    public:
        static T& instance() {// 返回单例对象（的引用）
            if (!t_value_) {// 不需要按照线程安全方式实现，因为每个线程都有一份
                t_value_ = new T();
                deleter_.set(t_value_);
            }
            return *t_value_;
        }

        static T* pointer() {// 返回单例对象（的指针）
            return t_value_;
        }

    }; // class ThreadLocalSingleton

} // namespace muduo


// 嵌套的Deleter类，借助POSIX线程库的TSD（线程特定数据）实现

#endif // MUDUO_BASE_THREADLOCALSINGLETON_H