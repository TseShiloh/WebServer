/*
Poller和EventLoop是组合关系
Poller对象的生存期由EventLoop控制
EventLoop的loop()函数通过调用Poller的poll()实现

 
Channel类是对I/O事件的注册域响应的封装
Channel的成员函数update()负责注册或更新I/O的可读可写等事件 -> 调用了EventLoop的updateChannel() -> 调用了Poller的updateChannel()
Channel的成员函数handleEvent()负责对所发生的I/O事件进行处理
一个EventLoop可以对应多个Channel，即一对多。而且是聚合关系，EventLoop不负责Channel的生存期控制
Channel不拥有文件描述符，Channel对象销毁的时候不会调用close关闭文件描述符
*/