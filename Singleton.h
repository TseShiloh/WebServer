#ifndef MUDUO_BASE_SINGLETON_H
#define MUDUO_BASE_SINGLETON_H

#include <boost/noncopyable.hpp>
#include <pthread.h>
#include <stdlib.h>

namespace muduo
{
    template<typename T>
    class Singleton
    {
    private:
        static pthread_once_t ponce_;// 该类型对象能保证“只被执行一次”
        static T*             value_;

        Singleton();
        ~Singleton();

        static void init();
        static void destroy();

    public:
        static T& instance();
    };

    static T& Singleton::instance() {
        pthread_once(&ponce_, %Singleton::init);// 保证该对象只创建了一次
        // 如果不用pthread_once就不是线程安全了的，当然可以用锁来实现，但是效率没有前者高。
        return *value_;
    }

    static void Singleton::init() {
        value_ = new T();
        ::atexit(destroy);// 登记了一个销毁函数destroy()
    }

    static void Singleton::destroy() {
        typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];// T必须得是完全类型，否则在编译器就报错
        delete value_;
    }
    
} // namespace muduo


/*  不完整类型

class base; struct test;
base和test只给出了声明，没有给出定义。
不完整类型必须通过某种方式补充完整，才能使用它们进行实例化，
否则只能用于定义指针或引用，因为此时实例化的是指针或引用本身，不是base或test对象。
用delete删除一个只有声明但无定义的类型的指针，是危险的。
这通常导致无法调用析构函数（包括对象本身的析构函数、成员/基类的析构函数），从而泄露资源。
    
*/
