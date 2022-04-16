# WebServer



## 功能

- 利用IO复用技术Epoll与线程池实现多线程的Reactor高并发模型；
- 利用正则与状态机解析HTTP请求报文，实现处理静态资源的请求；
- 利用标准库容器封装char，实现自动增长的缓冲区；
- 基于堆结构实现的定时器，关闭超时的非活动连接；
- 改进了线程池的实现，QPS提升了45%+；


## 致谢

@[markparticle](https://github.com/markparticle/WebServer/)


