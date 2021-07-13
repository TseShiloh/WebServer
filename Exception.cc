#include <muduo/base/Exception.h>
#include <cxxabi.h>
#include <execinfo.h>
#include <stdlib.h>
#include <stdio.h>

using namespace muduo;

Exception::Exception(const char* msg) : message_(msg) {
  fillStackTrace();
}

Exception::Exception(const string& msg) : message_(msg) {
  fillStackTrace();
}

Exception::~Exception() throw()
{
}

const char* Exception::what() const throw() {
  return message_.s_str();
}

const char* Exception::stackTrace() const throw() {
  return stack_.c_str();
}

void Exception::fillStackTrace() {
  const int len = 200;
  void* buffer[len];// 指针数组用来保存栈帧的地址
  int nptrs = ::backtrace(buffer, len);// 实际栈帧的个数
  char** strings = ::backtrace_symbols(buffer, nptrs);
  // backtrace函数获取当前线程的调用堆栈，获取的信息将会被存放在buffer中
  // backtrace_symbols函数将栈帧的“地址”转换成“名称”
  if(strings) {
    for (int i = 0; i < nptrs; ++i) {
      // TODO demangle function name with abi::__cxa_demangle
      stack_.append(demangle(strings[i]));
      //stack_.append(strings[i]);
      stack_.push_back('\n');
    }
    free(strings);
  } 
}

string Exception::demangle(const char* symbol)
{
  size_t size;
  int status;
  char temp[128];
  char* demangled;
  //first, try to demangle a c++ name
  if (1 == sscanf(symbol, "%*[^(]%*[^_]%127[^)+]", temp)) {
    if (NULL != (demangled = abi::__cxa_demangle(temp, NULL, &size, &status))) {
      string result(demangled);
      free(demangled);
      return result;
    }
  }
  //if that didn't work, try to get a regular c symbol
  if (1 == sscanf(symbol, "%127s", temp)) {
    return temp;
  }
 
  //if all else fails, just return the symbol
  return symbol;
}