#ifndef MUDUO_BASE_EXCEPTION_H
#define MUDUO_BASE_EXCEPTION_H

#include <muduo/base/Types.h>
#include <exception>

namespace muduo
{
    
class Exception : public std::exception
{
    public:
    explicit Exception(const char* what);
    explicit Exception(const string& what);
    virtual ~Exception() throw();
    virtual const char* what() const throw();
    const char* stackTrace() const throw();
    

    private:
    void fillStackTrace();
    string demangle(const char* symbol);

    string message_;
    string stack_;
};

} // namespace muduo

#endif // MUDUO_BASE_EXCEPTION_H