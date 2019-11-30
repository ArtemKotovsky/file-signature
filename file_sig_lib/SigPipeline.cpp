//
//  SigPipeline.cpp
//  file_signature
//
//  Created by artem k on 30.10.2019.
//  Copyright © 2019 artem k. All rights reserved.
//

#include "SigPipeline.hpp"

#include <algorithm>
#include <thread>
#include <fstream>

namespace file_sig
{
    SigPipeline::SigPipeline(ChunkReader& reader, Hasher hasher, uint32_t threadsCount)
        : m_hasher(std::move(hasher))
        , m_reader(reader)
    {
        threadsCount = std::max(threadsCount, 1u);
        m_pool.reserve(threadsCount);
        
        m_activeThreads = threadsCount;
        
        for (size_t i = 0; i < threadsCount; ++i)
        {
            m_pool.push_back(std::async(std::launch::async,
                                        &SigPipeline::hasherThread,
                                        this));
        }
    }

    SigPipeline::~SigPipeline()
    {
        m_records.setCleanup();
        waitAllThreads();
    }

    void SigPipeline::setRecordsCallback(RecordCb cb)
    {
        m_records.setOnRecordCallback(cb);
    }

    void SigPipeline::cancel(bool sync)
    {
        m_records.setCleanup();
        
        if (sync)
        {
            waitAllThreads();
            m_records.checkException();
        }
    }

    SigPipeline::WaitRes SigPipeline::wait(uint32_t timeoutMs)
    {
        return m_records.waitForResult(timeoutMs);
    }

    SigPipeline::WaitRes SigPipeline::wait(uint32_t timeoutMs, Record& record)
    {
        return m_records.tryPopRecord(timeoutMs, record);
    }

    void SigPipeline::hasherThread()
    {
        try
        {
            ChunkReader::Chunk chunk;
            
            while (m_reader.getNextChunk(chunk))
            {
                Record record;
                record.size = chunk.size();
                record.offset = chunk.offset();
                record.hash = m_hasher(chunk.data(), chunk.size());
                chunk.free();
                
                if (!m_records.pushRecord(std::move(record)))
                {
                    break;
                }
            }
        }
        catch (const std::exception&)
        {
            m_records.setException(std::current_exception());
        }
        
        if (1 == m_activeThreads--)
        {
            //
            // it is the latest thread
            // records will not be added anymore
            //
            m_records.setFreez();
        }
    }

    void SigPipeline::waitAllThreads() const
    {
        for (const auto& thread : m_pool)
        {
            thread.wait();
        }
    }
}
