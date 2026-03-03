#include "ThreadPool.h"

namespace happycat {

ThreadPool::ThreadPool(u32 threadCount) {
    m_Workers.reserve(threadCount);
    for (u32 i = 0; i < threadCount; i++) {
        m_Workers.emplace_back(&ThreadPool::WorkerThread, this);
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(m_QueueMutex);
        m_Stop = true;
    }
    m_Condition.notify_all();

    for (auto& worker : m_Workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::WorkerThread() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);
            m_Condition.wait(lock, [this] { return m_Stop || !m_Tasks.empty(); });

            if (m_Stop && m_Tasks.empty()) {
                return;
            }

            task = std::move(m_Tasks.front());
            m_Tasks.pop();
            m_ActiveTasks++;
        }

        task();

        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);
            m_ActiveTasks--;
            if (m_ActiveTasks == 0 && m_Tasks.empty()) {
                m_Finished.notify_all();
            }
        }
    }
}

void ThreadPool::WaitAll() {
    std::unique_lock<std::mutex> lock(m_QueueMutex);
    m_Finished.wait(lock, [this] { return m_Tasks.empty() && m_ActiveTasks == 0; });
}

} // namespace happycat
