## webserver介绍

这个webserver类是对整个web服务器的抽象，基于HTTPconnection类、timer类、epoller类、threadpool类实现一个完整的高性能web服务器的所有功能。

需要满足的功能有：

-   初始化服务器，为HTTP的连接做好准备；
-   处理每一个HTTP连接；
-   用定时器给每一个HTTP连接定时，并且处理掉过期的连接；
-   运用IO多路复用技术提升IO性能；
-   运用线程池技术提升服务器性能；

## webserver的逻辑

首先是进行服务器的初始化，进行各种参数设置。其中包括了事件模式的初始化、socket连接的建立过程，主要用到了以下两个函数：

```
bool initSocket_(); 
void initEventMode_(int trigMode);
```

在初始化socket的过程中，将listenFd\_描述符也加入epoll进行监视。这样的话，当监听的listenFd(socketFd)有新连接的时候，就会发来一个可读信号。同时，也将监听socket的行为(是否有新的连接)和监听每一个HTTP连接的行为(已经建立的连接有无IO请求)统一起来了，都抽象为了读写两个操作。所以我们就可以每一次直接处理所有的epoll就绪事件，在过程中对有新连接请求的情况单独分出来处理就可以了。

接下来开始处理HTTP连接。

最开始当然先清理过期的连接，并且得到下一次处理的时间。这个首先就需要在这个类中有一个TimerManager类型指针或者对象，用它调用`getNextHandle()`函数：

```
timeMS=timer_->getNextHandle();
```

然后得到所有已经就绪的事件，用一个循环处理所有的epoll就绪事件。在过程中需要分两种类型：收到新的HTTP请求和其他的读写请求。

1.  收到新的HTTP请求的情况

在`fd==listenFd_`的时候，也就是收到新的HTTP请求的时候，需要调用函数：

```
 void handleListen_();
```

这个函数中会得到新的描述符，然后需要将新的描述符和新的描述符对应的连接记录下来，调用下述函数即可：

```
void addClientConnection(int fd, sockaddr_in addr);
```

2.  已经建立连接的HTTP发来IO请求的情况

这种情况下，必然需要提供读写的处理，用下述两个函数完成：

```
void handleWrite_(HTTPconnection* client);
void handleRead_(HTTPconnection* client);
```

但是为了提高性能，使用了线程池，所以这两个函数就是将读写的底层实现函数加入到线程池，两个读写的底层实现函数为：

```
void onRead_(HTTPconnection* client);
void onWrite_(HTTPconnection* client);
```

epoll使用的是边缘触发ET，所以在读结束的时候根据需要改变epoll的事件。

## 边缘触发

### 1\. 对于读操作

（1）当缓冲区由不可读变为可读的时候，即缓冲区由空变为不空的时候。

（2）当有新数据到达时，即缓冲区中的待读数据变多的时候。

（3）当缓冲区有数据可读，且应用进程对相应的描述符进行`EPOLL_CTL_MOD` 修改`EPOLLIN`事件时。

### 2\. 对于写操作

（1）当缓冲区由不可写变为可写时。

（2）当有旧数据被发送走，即缓冲区中的内容变少的时候。

（3）当缓冲区有空间可写，且应用进程对相应的描述符进行`EPOLL_CTL_MOD` 修改`EPOLLOUT`事件时。

用函数：

```
void onProcess_(HTTPconnection* client);
```

来完成。

当一切都完成的时候，还需要一个关闭服务器的函数：

```
void closeConn_(HTTPconnection* client);   
```

## WebServer的实现

```
class WebServer {
public:
    WebServer(int port,int trigMode,int timeoutMS,bool optLinger,int threadNum);
    ~WebServer();

    void Start(); //一切的开始

private:
    //对服务端的socket进行设置，最后可以得到listenFd
    bool initSocket_(); 

    void initEventMode_(int trigMode);

    void addClientConnection(int fd, sockaddr_in addr); //添加一个HTTP连接
    void closeConn_(HTTPconnection* client);            //关闭一个HTTP连接

    void handleListen_();
    void handleWrite_(HTTPconnection* client);
    void handleRead_(HTTPconnection* client);

    void onRead_(HTTPconnection* client);
    void onWrite_(HTTPconnection* client);
    void onProcess_(HTTPconnection* client);

    void sendError_(int fd, const char* info);
    void extentTime_(HTTPconnection* client);

    static const int MAX_FD = 65536;
    static int setFdNonblock(int fd);

    int port_;
    int timeoutMS_;  /* 毫秒MS,定时器的默认过期时间 */
    bool isClose_;
    int listenFd_;
    bool openLinger_;
    char* srcDir_;//需要获取的路径

    uint32_t listenEvent_;
    uint32_t connectionEvent_;

    std::unique_ptr<TimerManager>timer_;
    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HTTPconnection> users_;
};
```