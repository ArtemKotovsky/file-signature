//
//  TaskPool.cpp
//  file_signature
//
//  Created by artem k on 30.10.2019.
//  Copyright © 2019 artem k. All rights reserved.
//

#include "TaskPool.hpp"

namespace utils
{
    TaskPool::TaskPool(size_t workerThreads)
        : m_stopped(false)
        , m_threadsCount(workerThreads)
    {
        m_threads.reserve(m_threadsCount);
        
        for (size_t i = 0; i < workerThreads; ++i)
        {
            m_threads.push_back(std::async(std::launch::async,
                                           &TaskPool::workerThread,
                                           this));
        }
    }

    TaskPool::~TaskPool()
    {
        stopImpl(true, true, false);
    }
        
    void TaskPool::push(Task task)
    {
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            
            if (m_stopped)
            {
                throw std::runtime_error("Cannot push a new task to the task pool, "
                                         "the task pool is stopped");
            }
            
            m_queue.push_back(std::move(task));
        }
        m_cv.notify_one();
    }

    void TaskPool::stop(bool cancelQueue, bool sync)
    {
        stopImpl(cancelQueue, sync, true);
    }

    size_t TaskPool::getThreadsCount() const
    {
        return m_threads.size();
    }

    void TaskPool::workerThread()
    {
        for (;;)
        {
            std::unique_lock<std::mutex> lock(m_mtx);
            
            m_cv.wait(lock, [this]()
            {
                return m_stopped || !m_queue.empty();
            });
            
            if (m_stopped && m_queue.empty())
            {
                //
                // stop was called,
                // need to finish the thread
                //
                return;
            }
            
            //
            // queue has a new task,
            // let's execute it
            //
            assert(!m_queue.empty());
            Task task = std::move(m_queue.front());
            m_queue.pop_front();
            
            //
            // execute task without lock
            // task is noexcept, so the caller side has to guarantee
            // the task will not throw anything
            //
            lock.unlock();
            
            try
            {
                task();
            }
            catch (...)
            {
                assert(!"A task must never throw an exception");
                std::terminate();
            }
            
            lock.lock();
        }
    }

    void TaskPool::stopImpl(bool cancelQueue, bool sync, bool checkThreads)
    {
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            m_stopped = true;
            
            if (cancelQueue)
            {
                m_queue.clear();
            }
        }
        
        m_cv.notify_all();

        if (sync)
        {
            //
            // wait until threads are finished
            //
            auto threads = std::move(m_threads);
            
            if (checkThreads)
            {
                //
                // get threads' results
                // a thread must not throw,
                // however, let's check this for debugging issues
                //
                for (auto& thread : threads)
                {
                     thread.get();
                }
            }
        }
    }
}
