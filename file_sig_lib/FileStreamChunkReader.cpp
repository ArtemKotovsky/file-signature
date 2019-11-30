//
//  FileStreamChunkReader.cpp
//  file_signature
//
//  Created by artem k on 18.11.2019.
//  Copyright © 2019 artem k. All rights reserved.
//

#include "FileStreamChunkReader.hpp"
#include "Exceptions.hpp"

#include <limits>
#include <thread>
#include <fstream>

namespace file_sig
{
    FileStreamChunkReader::FileStreamChunkReader(const std::string& fileName,
                                                 uint32_t cachedChunksCount,
                                                 uint32_t chunkSize)
    {
        m_file.open(fileName, std::ios::binary);
        THROW_IF(!m_file.is_open(), "Cannot open " << fileName);
        m_file.exceptions(std::ios::badbit);
        
        m_fileIo.resize(1024 * 1024);
        m_file.rdbuf()->pubsetbuf(m_fileIo.data(), m_fileIo.size());
        
        //
        // create 'cachedChunksCount' free chunks
        // each of that has pre allocated 'chunkSize' buffer
        //
        m_free.resize(cachedChunksCount, {std::vector<char>(chunkSize, 0), 0 });
        
        //
        // create a thread that reads file data
        //
        m_thread = std::async(std::launch::async,
                              &FileStreamChunkReader::readerThread,
                              this);
    }

    FileStreamChunkReader::~FileStreamChunkReader()
    {
        stop(false);
        m_thread.wait();
    }

    void FileStreamChunkReader::stop(bool sync)
    {
        //
        // stop the thread and clean up all cached data
        // the further reading is not necessary
        // caller wants to stop/cancel all operations
        //
        std::unique_lock<std::mutex> lock(m_chunkMutex);
        m_stopped = true;
        lock.unlock();
        
        //
        // notify all threads that wait for a new data
        // that it is finished
        //
        m_freeCv.notify_all();

        if (sync)
        {
            //
            // wait for thread stops
            // the mutex is needed for safety calling of this method
            // from multiple threads
            //
            std::lock_guard<std::mutex> threadLock(m_threadMutex);
            m_thread.get();
        }
    }

    bool FileStreamChunkReader::getChunk(const void *& data, uint32_t& size, uint64_t& offset)
    {
        std::unique_lock<std::mutex> lock(m_chunkMutex);
        
        m_readyCv.wait(lock, [this]()
        {
            return !m_ready.empty() || m_stopped || m_eof;
        });
        
        if (m_exception)
        {
            //
            // an exception happened in a reader thread
            //
            std::exception_ptr ex;
            std::swap(ex, m_exception);
            std::rethrow_exception(ex);
        }
        
        if (m_eof && m_ready.empty())
        {
            //
            // all the data have been read from a file
            // and all the read data have been processed
            // so need not further calls
            //
            m_stopped = true;
        }
        
        if (m_stopped)
        {
            return false;
        }
        
        //
        // get next ready chunk and move it to a busy queue
        //
        auto& val = m_ready.front();
        
        if (val.buffer.size() > std::numeric_limits<uint32_t>::max())
        {
            THROW("cannot convert " << val.buffer.size() << " to 32bits value");
        }
        
        data = static_cast<void*>(val.buffer.data());
        size = static_cast<uint32_t>(val.buffer.size());
        offset = val.offset;
        
        m_busy.splice(m_busy.end(), m_ready, m_ready.begin());
        return true;
    }

    void FileStreamChunkReader::freeChunk(const void * data, uint32_t /*size*/)
    {
        //
        // finds the chunk in the busy list
        // and move it to the free list
        // the chunk memory is not reallocated
        // and can be reused
        //
        std::lock_guard<std::mutex> lock(m_chunkMutex);
        
        for (auto it = m_busy.begin(); it != m_busy.end(); ++it)
        {
            if (data == it->buffer.data())
            {
                m_free.splice(m_free.end(), m_busy, it);
                m_freeCv.notify_one();
                return;
            }
        }
        
        //
        // just to detect a logical errors,
        // must never throw
        //
        THROW("Logic error, the buffer was not found");
    }

    void FileStreamChunkReader::readerThread()
    {
        try
        {
            std::unique_lock<std::mutex> lock(m_chunkMutex, std::defer_lock);
            
            while (!m_eof)
            {
                lock.lock();
                
                //
                // wait for free chunks
                // and move it from the free list
                //
                m_freeCv.wait(lock, [this]()
                {
                    return !m_free.empty() || m_stopped;
                });
                
                if (m_stopped)
                {
                    return;
                }
                
                auto var = std::move(m_free.back());
                m_free.pop_back();
                
                lock.unlock();
                
                //
                // read file data to a pre-allocated buffer of a free chunk
                //
                var.offset = m_file.tellg();
                m_file.read(var.buffer.data(), var.buffer.size());
                var.buffer.resize(m_file.gcount());
                
                lock.lock();
                
                //
                // move the chunk to the ready list
                // and notify about ready data
                //
                if (var.buffer.empty())
                {
                    m_eof = true;
                }
                else
                {
                    m_ready.push_back(std::move(var));
                    m_readyCv.notify_all();
                }
                
                if (m_file.eof())
                {
                    m_eof = true;
                }
                
                lock.unlock();
                m_readyCv.notify_all();
            }
        }
        catch (const std::exception&)
        {
            std::lock_guard<std::mutex> lock(m_chunkMutex);
            m_exception = std::current_exception();
            m_stopped = true;
            m_readyCv.notify_all();
        }
    }
}
