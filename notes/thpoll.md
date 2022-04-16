### 线程池是什么

下面这段是来自维基百科对线程池的定义：

> In computer programming, a thread pool is a software design pattern for achieving concurrency of execution in a computer program. Often also called a replicated workers or worker-crew model, a thread pool maintains multiple threads waiting for tasks to be allocated for concurrent execution by the supervising program.  

下图是线程池的原理示意：  

![](https://pic2.zhimg.com/v2-f9350f72ee96164cc85d8e09c4a3a0d9_b.jpg)

![](https://pic2.zhimg.com/80/v2-f9350f72ee96164cc85d8e09c4a3a0d9_720w.jpg)

  

线程池有两个核心的概念，一个是任务队列，一个是工作线程队列。任务队列负责存放主线程需要处理的任务，工作线程队列其实是一个死循环，负责从任务队列中取出和运行任务，可以看成是一个生产者和多个消费者的模型。

### 为什么需要线程池

可能有童鞋会好奇什么场合需要使用线程池，每次需要的时候创建一个新线程为何不可呢？  
题主最近在公司做的一个项目就使用到线程池，公司需要为某某超市做一套对每天进入超市的顾客做用户画像的系统。基本流程是：对海康摄像头拍摄到的每一帧画面做人脸检测，然后对每个人脸进行年龄、性别和特征点的计算，最后将结果post到服务端进行后续处理。大家都知道人脸相关算法耗时是较高的，如果所有计算任务都放在主线程进行，那么势必会阻塞主线程的处理流程，无法做到实时处理。使用多线程技术是大家自然而然想到的方案，对每一帧都创建一个新的线程来做这系列的处理是否合理呢？相信大家都知道，**线程的创建和销毁都是需要时间的**，在上述的场景中必然会频繁的创建和销毁线程，这样的开销相信是不能接受的，此时线程池技术便是很好的选择。  
另外，在一些高并发的网络应用中，线程池也是常用的技术。陈硕大神推荐的C++多线程服务端编程模式为：one loop per thread + thread pool，通常会有单独的线程负责接受来自客户端的请求，对请求稍作解析后将数据处理的任务提交到专门的计算线程池。

### 如何实现线程池

C++11提供了线程库（std::thread）以及一系列同步和互斥组件（std::condition\_variable和std::mutex），使得我们徒手造一个线程池的“轮子”方便许多。下面是用C++11实现的线程池，总共只需60行左右的代码。对C++11语法不太熟悉的童鞋，强烈建议大家通读一遍抄袭资料2中的README，对代码的每一步讲解非常清楚细致。

```cpp
#include <thread>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <queue>
#include <future>

class ThreadPool {
public:
    explicit ThreadPool(size_t threadNum) : stop_(false) {
        for(size_t i = 0; i < threadNum; ++i) {
            workers_.emplace_back([this]() {
                for(;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> ul(mtx_);
                        cv_.wait(ul, [this]() { return stop_ || !tasks_.empty(); });
                        if(stop_ && tasks_.empty()) { return; }
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> ul(mtx_);
            stop_ = true;
        }
        cv_.notify_all();
        for(auto& worker : workers_) {
            worker.join();
        }
    }

    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        auto taskPtr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        {
            std::unique_lock<std::mutex> ul(mtx_);
            if(stop_) { throw std::runtime_error("submit on stopped ThreadPool"); }
            tasks_.emplace([taskPtr]() { (*taskPtr)(); });
        }
        cv_.notify_one();
        return taskPtr->get_future();
    }

private:
    bool stop_;
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mtx_;
    std::condition_variable cv_;
};
```

### 如何使用

```text
ThreadPool pool(4);
std::vector<std::future<int>> results;
for(int i = 0; i < 8; ++i) {
    results.emplace_back(pool.submit([]() {
        // computing task and return result
    }));
}
for(auto && result: results)
    std::cout << result.get() << ' ';
```

### 抄袭资料

1.  [https://en.wikipedia.org/wiki/Thread\_pool](https://en.wikipedia.org/wiki/Thread_pool)
2.  [https://github.com/mtrebi/thread-pool](https://github.com/mtrebi/thread-pool)
3.  [https://github.com/progschj/Thr](https://github.com/progschj/ThreadPool)