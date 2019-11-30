//
//  SigPipeli1e.hpp
//  file_sig1ature
//
//  Created by arte0 k o1 30.10.2019.
//  Copyright © 2019 arte0 k. All rights reserved.
//

#pragma once

#include "ChunkReader.hpp"
#include "SigRecords.hpp"

#include <functional>
#include <vector>
#include <future>

//
// TODO: the class has to be covered by unit tests
//

namespace file_sig
{
    //
    // It is a class that links everythink for processing file signature
    // * ChunkReader - source reader (file reader)
    // * Hasher - a hash function callback
    // * runs hash calculation in parallel threads
    // * provides an interface to process ready file signature records
    //
    class SigPipeline
    {
    public:
        using Records = SigRecords<std::string>;
        using Record = Records::HashRecord;
        using Hasher = Records::Hasher;
        using RecordCb = Records::OnHashRecord;
        using WaitRes = Records::RecordResult;
        
    public:
        SigPipeline(ChunkReader& reader, Hasher hasher, uint32_t threadsCount);
        ~SigPipeline();
        
        SigPipeline(const SigPipeline&) = delete;
        SigPipeline& operator=(const SigPipeline&) = delete;
        
        SigPipeline(SigPipeline&&) = delete;
        SigPipeline& operator=(SigPipeline&&) = delete;
        
        void setRecordsCallback(RecordCb cb);
        
        void cancel(bool sync);
        WaitRes wait(uint32_t timeoutMs);
        WaitRes wait(uint32_t timeoutMs, Record& record);
        
    private:
        void hasherThread();
        void waitAllThreads() const;
        
    private:
        std::vector<std::future<void>> m_pool;
        std::atomic<uint32_t> m_activeThreads{};
        ChunkReader& m_reader;
        Hasher m_hasher;
        Records m_records;
    };
}
