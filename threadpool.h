#pragma once

#include <thread>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <queue>
#include <future>
#include <functional>

class ThreadPool {
private:
    bool m_stop;
    std::vector<std::thread> m_thread;
    std::queue<std::function<void()>> tasks;
    std::mutex m_mutex;
    std::condition_variable m_cv;

public:
    explicit ThreadPool(size_t threadNumber) :
        m_stop(false) {
        for (size_t i = 0; i < threadNumber; ++i) {
            m_thread.emplace_back(
                [this]() {
                    for (;;) {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lk(m_mutex); // lock
                            //在第二个参数 Predicate，只有当 pred 条件为 false 时调用 wait() 才会阻塞当前线程，
                            //并且在收到其他线程的通知后只有当 pred 为 true 时才会被解除阻塞。
                            m_cv.wait(lk, [this]() { return m_stop || !tasks.empty(); });
                            if (m_stop && tasks.empty()) return; //终止线程
                            task = std::move(tasks.front());     //取个任务
                            tasks.pop();
                        }
                        task();
                    }
                });
        }
    }

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;

    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lk(m_mutex);
            m_stop = true;
        }
        m_cv.notify_all();
        for (auto &threads : m_thread) {
            threads.join(); //阻塞主线程，等待子线程结束
        }
    }

    //提交一个函数要在池中异步执行
    template <typename F, typename... Args>
    auto submit(F &&f, Args &&...args) -> std::future<decltype(f(args...))> {
        auto taskPtr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        
        {
            std::unique_lock<std::mutex> lk(m_mutex);
            if (m_stop) throw std::runtime_error("submit on stopped ThreadPool");
            tasks.emplace([taskPtr]() { (*taskPtr)(); });
        }
        m_cv.notify_one();
        return taskPtr->get_future();
    }
};
