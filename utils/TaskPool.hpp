//
//  TaskPool.hpp
//  file_signature
//
//  Created by artem k on 30.10.2019.
//  Copyright © 2019 artem k. All rights reserved.
//

#pragma once

#include <functional>
#include <condition_variable>
#include <future>
#include <vector>
#include <deque>

namespace utils
{
    class TaskPool
    {
    public:
        //
        // noexcept is not a part of a function signature,
        // however it is a part of the current interface
        //
        using Task = std::function<void() noexcept>;
        
    public:
        explicit TaskPool(size_t workerThreads);
        ~TaskPool();
        
        TaskPool(const TaskPool&) = delete;
        TaskPool& operator=(const TaskPool&) = delete;
        
        TaskPool(TaskPool&&) = delete;
        TaskPool& operator=(TaskPool&&) = delete;
        
        void push(Task task);
        void stop(bool cancelQueue, bool sync);
        
        size_t getThreadsCount() const;
        
    private:
        void workerThread();
        void stopImpl(bool cancelQueue, bool sync, bool checkThreads);
        
    private:
        std::vector<std::future<void>> m_threads;
        std::deque<Task> m_queue;
        std::condition_variable m_cv;
        std::mutex m_mtx;
        bool m_stopped;
        const size_t m_threadsCount;
    };
}
