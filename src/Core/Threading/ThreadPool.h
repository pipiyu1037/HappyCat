#pragma once

#include "Core/Utils/Types.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <future>
#include <vector>

namespace happycat {

class ThreadPool {
public:
    explicit ThreadPool(u32 threadCount = std::thread::hardware_concurrency());
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // Submit a task
    template<typename F, typename... Args>
    auto Submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        using ReturnType = decltype(f(args...));

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<ReturnType> result = task->get_future();

        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);
            m_Tasks.emplace([task]() { (*task)(); });
        }

        m_Condition.notify_one();
        return result;
    }

    // Wait for all tasks
    void WaitAll();

    u32 GetThreadCount() const { return static_cast<u32>(m_Workers.size()); }

private:
    void WorkerThread();

    std::vector<std::thread> m_Workers;
    std::queue<std::function<void()>> m_Tasks;
    std::mutex m_QueueMutex;
    std::condition_variable m_Condition;
    std::condition_variable m_Finished;
    bool m_Stop = false;
    u32 m_ActiveTasks = 0;
};

} // namespace happycat
