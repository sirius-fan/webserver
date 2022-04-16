## buffer缓冲区的介绍

在这个项目中，客户端连接发来的HTTP请求以及回复给客户端所请求的资源，都需要缓冲区的存在。其实，在操作系统的内核中就有缓冲区的实现，read()/write()的调用就离不开缓冲区的支持。但是，在这里用缓冲区的实现不太方便。所以，在这个项目中实现了一个符合需要的缓冲区结构。

在C++的STL库中，vector容器其实就很适合作为缓冲区。为了能够满足我们的需要，我们以vector容器作为底层实体，在它的上面封装自己所需要的方法来实现一个自己的buffer缓冲区，满足读写的需要。

## buffer缓冲区的组成

省去每一个类必有的构造和析构函数，还需要以下部分：

### buffer的存储实体

缓冲区的最主要需要是读写数据的存储，也就是需要一个存储的实体。自己去写太繁琐了，直接用vector来完成。也就是buffer缓冲区里面需要一个：

```cpp
std::vector<char>buffer_;
```

### buffer所需要的变量

由于buffer缓冲区既要作为读缓冲区，也要作为写缓冲区，所以我们既需要指示当前读到哪里了，也需要指示当前写到哪里了。所以在buffer缓冲区里面设置变量：

```cpp
std::atomic<std::size_t>readPos_;
std::atomic<std::size_t>writePos_;
```

分别指示当前读写位置的下标。

### buffer所需要的方法

#### 读写接口

缓冲区最重要的就是读写接口，主要可以分为与客户端直接IO交互所需要的读写接口，以及收到客户端HTTP请求后，我们在处理过程中需要对缓冲区的读写接口。

1.  与客户端直接IO的读写借口

```cpp
ssize_t readFd(int fd,int* Errno);
ssize_t writeFd(int fd,int* Errno);
```

这个功能直接用read()/write()、readv()/writev()函数来实现。从某个连接接受数据的时候，有可能会超过vector的容量，所以我们用readv()来分散接受来的数据。当然，我们也可以用vector的动态扩容技术，但是代价比较高，不划算。需要往某个连接发送数据的时候，不用担心这个问题，调用write()就可以了。

最后无论是读还是写，结束之后都需要更新读指针和写指针的位置。

2.  处理HTTP连接过程中需要的读写接口

需要读buffer里面的数据，一般情况下也需要定义方法。但是在这里，我们用STL提供的对vector的方法和对string的支持就可以实现这些功能。所以，我们这部分主要需要实现向buffer缓冲区中添加数据的方法。

```cpp
void append(const char* str,size_t len);
void append(const std::string& str);
void append(const void* data,size_t len);
void append(const Buffer& buffer);
```

根据后续功能的需要，写了各种需要的实现。其中的具体功能可以参考具体的代码实现。

在往buffer缓冲区中添加数据也需要考虑超过容量的情况，也就是我们还需要实现这种情况下怎么动态扩容，怎么保证能够写入超过现有容量的数据，怎么分配新的空间。

也就是以下方法：

```cpp
void ensureWriteable(size_t len);
void allocateSpace(size_t len);
```

#### 更新定义变量的方法

在读写结束后，我们自然需要更新读指针和写指针的位置，也就是指示读写指针的变量的值。

```cpp
void updateReadPtr(size_t len);
void updateReadPtrUntilEnd(const char* end);
void updateWritePtr(size_t len);
void initPtr();
```

上述方法，分别为读指定长度后读指针的的更新方法，将读指针移到指定位置的方法，写入指定长度后写指针的更新方法和读写指针初始化的方法。

#### 获取信息的接口

这些都主要是为了上层的需求而实现的接口。

比如以下方法获取缓冲区中的信息：

```cpp
//缓存区中还可以读取的字节数
size_t writeableBytes() const;
//缓存区中还可以写入的字节数
size_t readableBytes() const;
//缓存区中已经读取的字节数
size_t readBytes() const;
```

以指针的方式获取当前读写指针：

```cpp
//获取当前读指针
const char* curReadPtr() const;
//获取当前写指针
const char* curWritePtrConst() const;
char* curWritePtr();
```

以及实现过程中需要的方法：

```cpp
 //返回指向缓冲区初始位置的指针
 char* BeginPtr_();
 const char* BeginPtr_() const;
```

#### 其他方法

比如：

```cpp
//将缓冲区的数据转化为字符串
std::string AlltoStr();
```

## buffer缓冲区的实现

```cpp
class Buffer{
public:
    Buffer(int initBufferSize=1024);
    ~Buffer()=default;

    //缓存区中可以读取的字节数
    size_t writeableBytes() const;
    //缓存区中可以写入的字节数
    size_t readableBytes() const;
    //缓存区中已经读取的字节数
    size_t readBytes() const;

    //获取当前读指针
    const char* curReadPtr() const;
    //获取当前写指针
    const char* curWritePtrConst() const;
    char* curWritePtr();
    //更新读指针
    void updateReadPtr(size_t len);
    void updateReadPtrUntilEnd(const char* end);//将读指针直接更新到指定位置
    //更新写指针
    void updateWritePtr(size_t len);
    //将读指针和写指针初始化
    void initPtr();

    //保证将数据写入缓冲区
    void ensureWriteable(size_t len);
    //将数据写入到缓冲区
    void append(const char* str,size_t len);
    void append(const std::string& str);
    void append(const void* data,size_t len);
    void append(const Buffer& buffer);

    //IO操作的读与写接口
    ssize_t readFd(int fd,int* Errno);
    ssize_t writeFd(int fd,int* Errno);

    //将缓冲区的数据转化为字符串
    std::string AlltoStr();

private:
    //返回指向缓冲区初始位置的指针
    char* BeginPtr_();
    const char* BeginPtr_() const;
    //用于缓冲区空间不够时的扩容
    void allocateSpace(size_t len);

    std::vector<char>buffer_; //buffer的实体
    std::atomic<std::size_t>readPos_;//用于指示读指针
    std::atomic<std::size_t>writePos_;//用于指示写指针

};
```